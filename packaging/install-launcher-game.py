#!/usr/bin/env python3
"""Añade un juego ya preparado a una biblioteca local y elige su portada."""
# SPDX-License-Identifier: GPL-2.0-or-later
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import uuid

from auto_cover import install_cover


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("prepared", type=Path, help="carpeta de juego ya preparada para mkxp-z")
    parser.add_argument("library", type=Path, help="carpeta switch/FangameNX/games local")
    args = parser.parse_args()
    source = args.prepared.resolve(strict=True)
    library = args.library.resolve(strict=True)
    if not source.is_dir() or not library.is_dir():
        parser.error("el origen y games deben ser carpetas existentes")
    metadata_path = source / "fangamenx.json"
    metadata = json.loads(metadata_path.read_text(encoding="utf-8")) if metadata_path.is_file() else {}
    if not isinstance(metadata, dict):
        parser.error("fangamenx.json debe ser un objeto")
    game_id = metadata.get("id", source.name)
    if not isinstance(game_id, str) or not re.fullmatch(r"[A-Za-z0-9_-]{1,80}", game_id):
        parser.error("el nombre de la carpeta debe ser un ID válido del launcher")
    target = library / game_id
    if target.exists():
        parser.error(f"ya existe {target}; no se sobrescribe una instalación")
    required = ("mkxp.json", "lib/nx-compat.rb", "lib/socket.rb")
    if any(not (source / path).is_file() for path in required):
        parser.error("falta mkxp.json o un shim obligatorio en lib/")
    if not ((source / "datos.zip").is_file() and (source / "Game.rgssad").is_file()) and not (
        (source / "Data/Scripts.rxdata").is_file()
        and (source / "Graphics").is_dir()
        and (source / "Audio").is_dir()
    ):
        parser.error("faltan datos preparados del juego")
    if any(path.is_symlink() for path in source.rglob("*")):
        parser.error("la carpeta preparada contiene enlaces simbólicos")
    if source == library or library in source.parents or source in library.parents:
        parser.error("origen y biblioteca no deben estar anidados")
    source_bytes = sum(path.stat().st_size for path in source.rglob("*") if path.is_file())
    if shutil.disk_usage(library).free < source_bytes + 64 * 1024 * 1024:
        parser.error("espacio libre insuficiente para copiar y verificar el juego")
    partial = library / ("." + game_id + ".partial-" + uuid.uuid4().hex)
    shutil.copytree(source, partial)
    originals = {p.relative_to(source) for p in source.rglob("*") if p.is_file()}
    copied = {p.relative_to(partial) for p in partial.rglob("*") if p.is_file()}
    if originals != copied or any(
        (source / rel).stat().st_size != (partial / rel).stat().st_size or
        digest(source / rel) != digest(partial / rel) for rel in originals
    ):
        raise SystemExit(f"Copia local incompleta en {partial}; se conserva para diagnóstico")
    cover = install_cover(partial, game_id)
    partial.rename(target)
    print(f"JUEGO={target}")
    print(f"PORTADA={cover}")


if __name__ == "__main__":
    main()
