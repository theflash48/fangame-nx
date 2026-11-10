#!/usr/bin/env python3
"""Diagnostica y prepara una copia local de RPG Maker XP para FangameNX."""
# SPDX-License-Identifier: GPL-2.0-or-later
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import stat
import sys
import unicodedata
import uuid
import zipfile

from auto_cover import install_cover

ROOT = Path(__file__).resolve().parents[1]
LOCAL = ROOT / "local"
RESOURCES = {"Data", "Graphics", "Audio", "Fonts", "Movies"}
EXCLUDE_DIRS = {"pbs", "databackup", "dataexport", "dist", "credits"}
SKIP_EXT = {".exe", ".dll", ".pdf", ".lnk", ".rar", ".zip", ".7z"}
SKIP_NAMES = {"thumbs.db", "desktop.ini", ".nomedia"}
GAME_KEYS = {
    "windowTitle", "defScreenW", "defScreenH", "rgssVersion", "SESourceCount",
    "BGMTrackCount", "midiSoundFont", "midiChorus", "midiReverb",
    "subImageFix", "enableBlitting", "maxTextureSize", "bindingNames",
    "inputRepeatStart", "inputRepeatDelay", "patches", "execName",
    "solidFonts", "fontSub", "useScriptNames", "JITEnable", "JITMaxCache",
    "JITMinCalls",
}
MAX_FILES = 100_000
MAX_BYTES = 8 * 1024**3
MAX_CONTAINER_BYTES = 4 * 1024**3 - 64 * 1024**2


def sha(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def json5_object(data: bytes) -> dict:
    # El contrato existente de Empaquetar.ps1 admite comentarios y comas
    # finales; las cadenas se recorren sin tocar http:// ni escapes.
    text = data.decode("utf-8-sig")
    out = []
    i = 0
    quoted = escaped = False
    while i < len(text):
        c = text[i]
        if quoted:
            out.append(c)
            if escaped:
                escaped = False
            elif c == "\\":
                escaped = True
            elif c == '"':
                quoted = False
        elif c == '"':
            quoted = True
            out.append(c)
        elif text.startswith("//", i):
            i = text.find("\n", i)
            if i < 0:
                break
            out.append("\n")
        elif text.startswith("/*", i):
            end = text.find("*/", i + 2)
            if end < 0:
                raise ValueError("comentario JSON5 sin cierre")
            i = end + 1
        else:
            out.append(c)
        i += 1
    text = "".join(out)
    out = []
    i = 0
    quoted = escaped = False
    while i < len(text):
        c = text[i]
        if quoted:
            out.append(c)
            if escaped:
                escaped = False
            elif c == "\\":
                escaped = True
            elif c == '"':
                quoted = False
        elif c == '"':
            quoted = True
            out.append(c)
        elif c == "," and text[i + 1:].lstrip().startswith(("}", "]")):
            pass
        else:
            out.append(c)
        i += 1
    value = json.loads("".join(out))
    if not isinstance(value, dict):
        raise ValueError("se esperaba un objeto JSON")
    return value


def safe_name(name: str) -> bool:
    if not name or "\\" in name or name.startswith("/") or "\x00" in name:
        return False
    parts = name.split("/")
    if any(p in {"", ".", ".."} or len(p) > 255 or p.endswith((" ", ".")) or ":" in p for p in parts):
        return False
    return not any(re.fullmatch(r"(?i)(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\..*)?", p) for p in parts)


class Source:
    def __init__(self, path: Path):
        self.path = path.resolve(strict=True)
        self.archive = zipfile.ZipFile(self.path) if self.path.is_file() else None
        self.files: dict[str, object] = {}
        self.folded: dict[str, str] = {}
        self.parents: set[str] = set()
        if self.archive:
            raw = [i for i in self.archive.infolist() if not i.is_dir()]
            if len(raw) > MAX_FILES or sum(i.file_size for i in raw) > MAX_BYTES:
                raise ValueError("ZIP excede el límite de entradas o tamaño")
            seen_zip: set[str] = set()
            for entry in self.archive.infolist():
                if not safe_name(entry.filename.rstrip("/")) or entry.flag_bits & 1:
                    raise ValueError(f"entrada ZIP insegura: {entry.filename}")
                folded_name = entry.filename.rstrip("/").casefold()
                if folded_name in seen_zip:
                    raise ValueError(f"entrada ZIP duplicada o conflictiva: {entry.filename}")
                seen_zip.add(folded_name)
                mode = stat.S_IFMT(entry.external_attr >> 16)
                if mode not in (0, stat.S_IFREG, stat.S_IFDIR):
                    raise ValueError(f"tipo de entrada ZIP no admitido: {entry.filename}")
            names = [i.filename for i in raw]
            prefixes = {n.split("/")[0] for n in names if "/" in n}
            prefix = ""
            if "Game.ini" not in names and len(prefixes) == 1:
                candidate = next(iter(prefixes)) + "/"
                if candidate + "Game.ini" in names:
                    prefix = candidate
            for entry in raw:
                if not entry.filename.startswith(prefix):
                    raise ValueError("ZIP con varias raíces; indica una carpeta de juego")
                relative = entry.filename[len(prefix):]
                if not relative:
                    continue
                self._add(relative, entry)
            if any(name.casefold().rstrip("/") in self.folded for name in
                   (i.filename[len(prefix):] for i in self.archive.infolist() if i.is_dir() and i.filename.startswith(prefix))):
                raise ValueError("ZIP con archivo y directorio del mismo nombre")
        elif self.path.is_dir():
            for file in self.path.rglob("*"):
                if file.is_symlink():
                    raise ValueError(f"enlace no admitido: {file}")
                if file.is_file():
                    self._add(file.relative_to(self.path).as_posix(), file)
        else:
            raise ValueError("la fuente debe ser una carpeta o archivo ZIP")
        if len(self.files) > MAX_FILES or sum(self.size(n) for n in self.files) > MAX_BYTES:
            raise ValueError("fuente demasiado grande")

    def _add(self, name: str, origin: object) -> None:
        if not safe_name(name):
            raise ValueError(f"ruta insegura: {name}")
        folded = name.casefold()
        if folded in self.folded or folded in self.parents:
            raise ValueError(f"ruta duplicada o conflictiva: {name}")
        parts = folded.split("/")
        if any("/".join(parts[:i]) in self.folded for i in range(1, len(parts))):
            raise ValueError(f"archivo usado como directorio: {name}")
        self.files[name] = origin
        self.folded[folded] = name
        self.parents.update("/".join(parts[:i]) for i in range(1, len(parts)))

    def close(self) -> None:
        if self.archive:
            self.archive.close()

    def find(self, name: str) -> str | None:
        return self.folded.get(name.casefold())

    def has_path(self, name: str) -> bool:
        folded = name.casefold().rstrip("/")
        return folded in self.folded or folded in self.parents

    def fingerprint(self) -> str:
        if self.archive:
            return sha(self.path)
        h = hashlib.sha256()
        for name in sorted(self.files):
            h.update(name.encode("utf-8"))
            h.update(str(self.size(name)).encode("ascii"))
            h.update(sha(self.files[name]).encode("ascii"))
        return h.hexdigest()

    def size(self, name: str) -> int:
        origin = self.files[name]
        return origin.file_size if isinstance(origin, zipfile.ZipInfo) else origin.stat().st_size

    def open(self, name: str):
        origin = self.files[name]
        return self.archive.open(origin) if self.archive else origin.open("rb")

    def read(self, name: str, limit: int = 1024 * 1024) -> bytes:
        if self.size(name) > limit:
            raise ValueError(f"archivo de configuración demasiado grande: {name}")
        with self.open(name) as f:
            return f.read(limit + 1)

    def copy(self, name: str, target: Path) -> None:
        target.parent.mkdir(parents=True, exist_ok=True)
        with self.open(name) as src, target.open("xb") as dest:
            shutil.copyfileobj(src, dest, 1024 * 1024)


def slug(text: str) -> str:
    normalized = unicodedata.normalize("NFKD", text).replace("ñ", "n").replace("Ñ", "N")
    ascii_text = "".join(c for c in normalized if not unicodedata.combining(c))
    return re.sub(r"-+", "-", re.sub(r"[^a-z0-9]", "-", ascii_text.lower())).strip("-")[:80]


def game_ini(data: bytes) -> dict[str, str]:
    try:
        text = data.decode("utf-8-sig")
    except UnicodeDecodeError:
        text = data.decode("cp1252")
    section = ""
    result = {}
    for line in text.splitlines():
        line = line.strip()
        if line.startswith("[") and line.endswith("]"):
            section = line[1:-1].casefold()
        elif section == "game" and "=" in line:
            key, value = line.split("=", 1)
            result[key.strip().casefold()] = value.strip()
    return result


def profile_for(source: Source, title: str, selected: str) -> tuple[str, dict]:
    profiles = []
    for file in sorted((ROOT / "juegos").glob("*/perfil.json")):
        if file.parent.name == "generico":
            continue
        profile = json.loads(file.read_text(encoding="utf-8"))
        if selected != "auto" and file.parent.name != selected:
            continue
        detect = profile.get("detectar") or {}
        pattern = detect.get("titulo")
        paths = detect.get("ficheros") or []
        title_match = bool(pattern and re.search(pattern, title, re.I))
        files_match = bool(paths and all(source.has_path(p.replace("\\", "/")) for p in paths))
        score = 3 if title_match and files_match else 2 if title_match else 1 if files_match else 0
        if selected != "auto" or score:
            profiles.append((score, file.parent.name, profile))
    if selected != "auto" and not profiles:
        raise ValueError(f"perfil no encontrado: {selected}")
    if not profiles:
        return "generico", json.loads((ROOT / "juegos/generico/perfil.json").read_text(encoding="utf-8"))
    profiles.sort(reverse=True, key=lambda x: x[0])
    if selected == "auto" and len(profiles) > 1 and profiles[0][0] == profiles[1][0]:
        raise ValueError("varios perfiles coinciden; usa --profile")
    return profiles[0][1], profiles[0][2]


def diagnose(source: Source, profile_name: str, family: str | None, game_id: str | None,
             name: str | None, author: str | None, version: str | None) -> dict:
    problems = []
    warnings = []
    ini_name = source.find("Game.ini")
    ini = game_ini(source.read(ini_name, 65536)) if ini_name else {}
    if not ini_name:
        problems.append("Falta Game.ini en la raíz del juego")
    title = ini.get("title", "")
    if not title:
        problems.append("Falta Title= en Game.ini")
    selected_name, profile = profile_for(source, title, profile_name)
    actual_family = family or profile.get("motor")
    if not actual_family and ini.get("library", "").upper().startswith("RGSS10"):
        actual_family = "rgss1"
        warnings.append("Familia rgss1 inferida de Library=; comprobar scripts y compatibilidad Ruby 1.8")
    if actual_family not in {"ruby31", "ruby18", "rgss1"}:
        problems.append("Familia Ruby desconocida; indica --family")
    if actual_family in {"ruby18", "rgss1"}:
        warnings.append("Scripts antiguos: el runtime ejecuta Ruby 3.1; posible transformación NXFuente")
    names = source.files
    for directory in ("Data", "Graphics", "Audio"):
        if not any(n.casefold().startswith(directory.casefold() + "/") for n in names):
            problems.append(f"Falta {directory}/")
    resource_bytes = sum(source.size(n) for n in names if
                         n.split("/")[0].casefold() in {d.casefold() for d in RESOURCES})
    if resource_bytes > MAX_CONTAINER_BYTES:
        problems.append("Recursos demasiado grandes para datos.zip sin ZIP64 verificado")
    script_path = ini.get("scripts", "Data/Scripts.rxdata").replace("\\", "/")
    if not safe_name(script_path):
        problems.append("Scripts= contiene una ruta insegura")
    elif not source.find(script_path) and not any(n.casefold().startswith("data/scripts/") for n in names):
        problems.append(f"No se encontró el archivo o carpeta de scripts: {script_path}")
    if source.find("Game.rgssad"):
        warnings.append("Archivo Game.rgssad existente: se conservará; revisar si está cifrado")
    other_archives = [n for n in names if "/" not in n and
                      n.casefold().endswith((".rgssad", ".rgss2a", ".rgss3a")) and
                      n.casefold() != "game.rgssad"]
    if other_archives:
        problems.append("Archivo RGSS con nombre no estándar; requiere adaptación específica: " +
                        ", ".join(other_archives[:3]))
    if (profile.get("mkxp") or {}).get("execName") not in (None, "Game"):
        problems.append("El perfil usa execName distinto de Game; revisar alias de INI y archivo RGSS")
    if source.find("mkxp.json") is None:
        warnings.append("Sin mkxp.json propio; se usará la base del runtime")
    if any(not n.isascii() for n in names):
        warnings.append("Hay nombres no ASCII; verificar su copia cuando se transfiera por MTP")
    identifier = game_id or profile.get("slug") or slug(title)
    if not re.fullmatch(r"[A-Za-z0-9_-]{1,80}", identifier or ""):
        problems.append("No se pudo obtener un ID válido; indica --id")
    display_name = name or profile.get("nombre") or title
    for label, value, limit in (("name", display_name, 96), ("author", author or "", 96),
                                ("version", version or "", 40)):
        if len(value.encode("utf-8")) > limit:
            problems.append(f"Metadato {label} supera {limit} bytes")
    if selected_name == "generico":
        warnings.append("Perfil genérico: compatibilidad no probada")
    for needed in (ROOT / "runtime/mkxp.base.json", ROOT / "runtime/shims/catalogo.json",
                   ROOT / "runtime/lib/nx-compat.rb", ROOT / "runtime/lib/socket.rb",
                   ROOT / "src/ruby-3.1.7/lib"):
        if not needed.exists():
            problems.append(f"Dependencia local ausente: {needed.relative_to(ROOT)}")
    for name in profile.get("preload") or []:
        if not safe_name(name) or "/" in name or not (ROOT / "juegos" / selected_name / "lib" / name).is_file():
            problems.append(f"Shim del perfil no disponible: {name}")
    estimated = sum(source.size(n) for n in names)
    return {
        "id": identifier, "title": title, "name": display_name,
        "author": author or "", "version": version or "", "profile": selected_name,
        "family": actual_family, "source_files": len(names),
        "estimated_source_bytes": estimated, "scripts": script_path,
        "resources": {d: sum(n.casefold().startswith(d.casefold() + "/") for n in names) for d in RESOURCES},
        "problems": problems, "warnings": warnings,
        "ready_for_staging": not problems,
    }


def runtime_config(source: Source, report: dict, profile: dict, stage: Path) -> None:
    cfg = json5_object((ROOT / "runtime/mkxp.base.json").read_bytes())
    own = source.find("mkxp.json")
    if own:
        original = json5_object(source.read(own, 512 * 1024))
        cfg.update({key: val for key, val in original.items() if key in GAME_KEYS})
    catalog = json5_object((ROOT / "runtime/shims/catalogo.json").read_bytes())["shims"]
    wanted = set(profile.get("shims") or [])
    preload = []
    for key, shim in sorted(catalog.items(), key=lambda item: item[1].get("orden", 0)):
        if not (shim.get("siempre") or report["family"] in shim.get("familias", []) or key in wanted):
            continue
        files = shim.get("ficheros", [])
        for name in files:
            if shim.get("en") == "shims":
                source_file = ROOT / "runtime/shims" / name
                dest = stage / "lib" / name
                dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(source_file, dest)
            if shim.get("carga") == "preload":
                preload.append("lib/" + name)
        for key2, val in (shim.get("mkxp") or {}).items():
            cfg[key2] = val
    cfg.update(profile.get("mkxp") or {})
    for name in profile.get("preload") or []:
        if not safe_name(name) or "/" in name:
            raise ValueError(f"preload de perfil no válido: {name}")
        preload.append("lib/" + name)
    if source.find("preload.rb"):
        preload.append("preload.rb")
    cfg["preloadScript"] = preload
    cfg["rubyLoadpath"] = list(dict.fromkeys(["lib", *(profile.get("rubyLoadpath") or [])]))
    cfg.setdefault("windowTitle", report["title"])
    (stage / "mkxp.json").write_text(json.dumps(cfg, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def build(source: Source, report: dict, profile: dict, output: Path) -> dict:
    output = output.resolve()
    if LOCAL not in output.parents or output == LOCAL:
        raise ValueError("el staging debe quedar dentro de local/")
    manifest_path = output.parent / (output.name + ".manifest.json")
    source_hash = source.fingerprint()
    if output.exists():
        if not manifest_path.is_file():
            raise FileExistsError(f"staging existente sin manifiesto: {output}")
        recorded = json.loads(manifest_path.read_text(encoding="utf-8"))
        if recorded.get("source_sha256") != source_hash:
            raise ValueError("el staging existente procede de otra fuente; se conserva")
        if any(recorded.get(key) != report[key] for key in
               ("id", "profile", "family", "name", "author", "version")):
            raise ValueError("el staging existente usa otras opciones; se conserva")
        if not all(safe_name(item["path"]) for item in recorded["files"]):
            raise ValueError("manifiesto existente con ruta insegura")
        actual = {p.relative_to(output).as_posix() for p in output.rglob("*") if p.is_file()}
        if actual != {item["path"] for item in recorded["files"]} or not all(
                   (output / item["path"]).is_file() and
                   (output / item["path"]).stat().st_size == item["size"] and
                   sha(output / item["path"]) == item["sha256"] for item in recorded["files"]):
            raise ValueError("el staging existente no coincide con su manifiesto; se conserva")
        return {"stage": str(output), "reused": True, "manifest": str(manifest_path)}
    if manifest_path.exists():
        raise FileExistsError(f"manifiesto existente sin staging: {manifest_path}")
    if source.path.is_dir() and (source.path == output or source.path in output.parents or output in source.path.parents):
        raise ValueError("fuente y staging no pueden estar anidados")
    output.parent.mkdir(parents=True, exist_ok=True)
    required = report["estimated_source_bytes"] + 256 * 1024**2
    if shutil.disk_usage(output.parent).free < required:
        raise OSError(f"espacio insuficiente; se estiman al menos {required} bytes libres")
    if source.archive:
        bad = source.archive.testzip()
        if bad is not None:
            raise ValueError(f"CRC ZIP incorrecto: {bad}")
    partial = output.parent / ("." + output.name + ".partial-" + uuid.uuid4().hex)
    partial.mkdir()
    try:
        lib = partial / "lib"
        lib.mkdir()
        for origin in (ROOT / "runtime/lib").rglob("*"):
            if origin.is_file():
                target = lib / origin.relative_to(ROOT / "runtime/lib")
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(origin, target)
        stdlib = ROOT / "src/ruby-3.1.7/lib"
        for line in (ROOT / "runtime/lib-stdlib.txt").read_text(encoding="utf-8").splitlines():
            name = line.strip()
            if not name or name.startswith("#"):
                continue
            origin = stdlib / name
            if not origin.is_file():
                raise FileNotFoundError(f"falta biblioteca estándar: {origin}")
            target = lib / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(origin, target)
        for name in profile.get("preload") or []:
            origin = ROOT / "juegos" / report["profile"] / "lib" / name
            if not origin.is_file():
                raise FileNotFoundError(f"falta shim de perfil: {origin}")
            shutil.copyfile(origin, lib / name)
        exclusions = EXCLUDE_DIRS | {x.casefold() for x in profile.get("excluir") or []}
        loose = {x.replace("\\", "/").casefold() for x in profile.get("sueltos") or []}
        loose.add("data/scripts")
        packed = []
        for name in sorted(source.files):
            parts = name.split("/")
            if parts[0].casefold() in exclusions or name.casefold() in SKIP_NAMES:
                continue
            if Path(name).suffix.casefold() in SKIP_EXT or name.casefold().endswith(".parcial"):
                continue
            if len(parts) == 1 and (name.casefold() in {"mkxp.json", "fangamenx.json"} or
                                    re.fullmatch(r"(?i)Game.*\.rxdata", name)):
                continue
            target_name = name
            for canonical in RESOURCES:
                if parts[0].casefold() == canonical.casefold():
                    target_name = "/".join([canonical, *parts[1:]])
                    break
            if len(parts) == 1 and name.casefold() in {"game.ini", "game.rgssad"}:
                target_name = "Game.ini" if name.casefold() == "game.ini" else "Game.rgssad"
            if parts[0].casefold() in {x.casefold() for x in RESOURCES} and not any(
                name.casefold().startswith(item + "/") for item in loose
            ):
                packed.append((name, target_name))
            else:
                target = partial / target_name
                if target.exists():
                    raise ValueError(f"colisión con el runtime: {name}")
                source.copy(name, target)
        if not packed:
            raise ValueError("sin recursos para datos.zip")
        with zipfile.ZipFile(partial / "datos.zip", "w", compression=zipfile.ZIP_STORED, allowZip64=False) as archive:
            for name, target_name in packed:
                with source.open(name) as src, archive.open(target_name, "w") as dest:
                    shutil.copyfileobj(src, dest, 1024 * 1024)
        if not source.find("Game.rgssad"):
            (partial / "Game.rgssad").write_bytes(b"RGSSAD\x00\x01")
        runtime_config(source, report, profile, partial)
        manifest = {
            "id": report["id"], "source_kind": "zip" if source.archive else "folder",
            "source_sha256": source_hash, "profile": report["profile"],
            "family": report["family"], "name": report["name"],
            "author": report["author"], "version": report["version"],
            "warnings": report["warnings"],
            "transformations": ["ZIP_STORED de recursos",
                                *( ["Data/Scripts suelta"] if source.has_path("Data/Scripts") else [] ),
                                "base + mkxp permitido + perfil + shims", "portada automática"],
        }
        metadata = {
            "id": report["id"], "name": report["name"],
            "author": report["author"], "version": report["version"],
            "description": "Preparado para FangameNX; compatibilidad pendiente de prueba.",
        }
        (partial / "fangamenx.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        cover = install_cover(partial)
        files = sorted(p for p in partial.rglob("*") if p.is_file())
        manifest["cover"] = cover
        manifest["files"] = [
            {"path": p.relative_to(partial).as_posix(), "size": p.stat().st_size, "sha256": sha(p)}
            for p in files
        ]
        if source.fingerprint() != source_hash:
            raise ValueError("la fuente cambió durante la preparación; el parcial se conserva")
        partial.rename(output)
        manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        return {"stage": str(output), "files": len(files), "resources": len(packed),
                "manifest": str(manifest_path), "cover": cover, "reused": False}
    except Exception:
        # Los parciales quedan privados para diagnóstico o reintento con otro destino.
        raise


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="carpeta o ZIP de un juego propio")
    parser.add_argument("--diagnose", action="store_true", help="solo analiza; no genera paquete")
    parser.add_argument("--output", type=Path, help="staging nuevo dentro de local/")
    parser.add_argument("--profile", default="auto", help="perfil de juegos/ o auto")
    parser.add_argument("--family", choices=("ruby31", "ruby18", "rgss1"))
    parser.add_argument("--id", help="ID de carpeta para la biblioteca")
    parser.add_argument("--name", help="nombre visible, si difiere del título o perfil")
    parser.add_argument("--author", help="autor según la fuente autorizada; no se infiere")
    parser.add_argument("--version", help="versión del juego si se conoce")
    args = parser.parse_args()
    if not args.diagnose and not args.output:
        parser.error("indica --diagnose o --output")
    source = Source(args.source)
    try:
        report = diagnose(source, args.profile, args.family, args.id,
                          args.name, args.author, args.version)
        print(json.dumps(report, ensure_ascii=False, indent=2))
        if args.diagnose or not report["ready_for_staging"]:
            return 0 if report["ready_for_staging"] else 2
        _, profile = profile_for(source, report["title"], args.profile)
        result = build(source, report, profile, args.output)
        print(json.dumps(result, ensure_ascii=False, indent=2))
        return 0
    finally:
        source.close()


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, zipfile.BadZipFile, json.JSONDecodeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        sys.exit(2)
