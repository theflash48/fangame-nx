#!/usr/bin/env python3
"""Añade un juego ya preparado a una biblioteca local y elige su portada."""
# SPDX-License-Identifier: GPL-2.0-or-later
import argparse
from pathlib import Path
import re
import shutil

from auto_cover import install_cover


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("prepared", type=Path, help="carpeta de juego ya preparada para mkxp-z")
    parser.add_argument("library", type=Path, help="carpeta switch/FangameNX/games local")
    args = parser.parse_args()
    source = args.prepared.resolve(strict=True)
    library = args.library.resolve(strict=True)
    if not source.is_dir() or not library.is_dir():
        parser.error("el origen y games deben ser carpetas existentes")
    if not re.fullmatch(r"[A-Za-z0-9_-]{1,80}", source.name):
        parser.error("el nombre de la carpeta debe ser un ID válido del launcher")
    target = library / source.name
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
    shutil.copytree(source, target)
    print(f"JUEGO={target}")
    print(f"PORTADA={install_cover(target)}")


if __name__ == "__main__":
    main()
