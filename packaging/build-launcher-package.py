#!/usr/bin/env python3
"""Prepara una entrega local del launcher. Nunca escribe en una microSD ni en MTP."""
# SPDX-License-Identifier: GPL-2.0-or-later
from hashlib import sha256
from pathlib import Path
import json
import shutil

ROOT = Path(__file__).resolve().parents[1]
NRO = ROOT / "mkxp-z-nx" / "FangameNX.nro"
OUT = ROOT / "local" / "m013" / ("package-" + sha256(NRO.read_bytes()).hexdigest()[:10]) if NRO.is_file() else ROOT / "local" / "m013" / "package-missing-nro"
APP = OUT / "switch" / "FangameNX"


def copy(source: Path, target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)


def fixture(name: str, files: dict[str, str], metadata: dict) -> None:
    dest = APP / "games" / name
    source = ROOT / "docs" / "fangame-nx" / "fixtures" / name
    for from_name, to_name in files.items():
        copy(source / from_name, dest / to_name)
    for folder in ("Audio", "Graphics", "Data", "lib"):
        (dest / folder).mkdir(parents=True, exist_ok=True)
    (dest / "Data" / "Scripts.rxdata").write_bytes(b"")
    copy(ROOT / "runtime/lib/nx-compat.rb", dest / "lib/nx-compat.rb")
    copy(ROOT / "runtime/lib/socket.rb", dest / "lib/socket.rb")
    (dest / "fangamenx.json").write_text(
        json.dumps({"id": name, **metadata}, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


def main() -> None:
    if OUT.exists():
        raise SystemExit(f"Ya existe {OUT}; se conserva sin sobrescribir")
    nro = NRO
    elf = ROOT / "mkxp-z-nx" / "FangameNX.elf"
    if not nro.is_file() or not elf.is_file():
        raise SystemExit("Compila primero con FNX_LAUNCHER=1")
    (APP / "config").mkdir(parents=True)
    (APP / "logs").mkdir()
    copy(nro, APP / "FangameNX.nro")
    (APP / "config" / "launcher.json").write_text('{"schema":1,"compact":false}\n', encoding="utf-8")
    fixture("fnx009", {
        "interactive.rb": "interactive.rb", "mkxp.json": "mkxp.json",
        "Game.ini": "Game.ini", "entrada.txt": "entrada.txt",
    }, {"name": "Pulso", "description": "Prueba original de dibujo, entrada A/X/B y salida.",
        "author": "FangameNX", "version": "M013"})
    fixture("fnx017", {
        "selector.rb": "selector.rb", "scene_a.rb": "fixtures/a/scene.rb",
        "scene_b.rb": "fixtures/b/scene.rb", "entrada_a.txt": "fixtures/a/entrada.txt",
        "entrada_b.txt": "fixtures/b/entrada.txt", "siguiente.txt": "siguiente.txt",
        "mkxp.json": "mkxp.json", "Game.ini": "Game.ini",
    }, {"name": "Órbita doble", "description": "Dos escenas sintéticas que alternan entre ejecuciones.",
        "author": "FangameNX", "version": "M013"})
    copy(ROOT / "LICENSE", OUT / "LICENSE")
    copy(ROOT / "LICENCIAS.md", OUT / "LICENCIAS.md")
    (OUT / "INSTALAR-DESPUES.txt").write_text(
        "Paquete local de desarrollo FangameNX. No instalado.\n"
        "Tras la revisión del propietario, copiar switch/FangameNX/ a la raíz de una microSD propia.\n"
        "Abrir FangameNX.nro desde Homebrew Menu con memoria de aplicación.\n"
        "A confirma, B vuelve, X reescanea, Y abre ajustes y + sale.\n"
        "La biblioteca, los detalles y los ajustes admiten controles tactiles.\n"
        "Durante un juego, mantener L+R+PLUS unos 0,8 s para abrir el menu rapido.\n"
        "Volver a la biblioteca pide confirmacion; B sigue disponible para el juego.\n"
        "La velocidad se guarda por juego en config/launcher.json.\n"
        "Controles del juego: elegir entrada A/B/X/Y, pulsar el boton fisico; los conflictos intercambian asignaciones.\n"
        "PLUS cancela la captura; restaurar recupera la distribucion de ese juego.\n"
        "Los ejemplos son sintéticos originales; no se afirma compatibilidad con fangames.\n",
        encoding="utf-8",
    )
    files = sorted(p for p in OUT.rglob("*") if p.is_file())
    manifest = OUT / "MANIFEST.sha256.txt"
    manifest.write_text("".join(
        f"{sha256(p.read_bytes()).hexdigest()}  {p.relative_to(OUT).as_posix()}\n"
        for p in files
    ), encoding="utf-8")
    print(f"PACKAGE={OUT}")
    print(f"FILES={len(files)}")
    print(f"NRO_SHA256={sha256(nro.read_bytes()).hexdigest()}")
    print(f"MANIFEST_SHA256={sha256(manifest.read_bytes()).hexdigest()}")


if __name__ == "__main__":
    main()
