#!/usr/bin/env python3
"""Prepara CRuby 3.1.7 con los parches versionados para el build Switch.

Solo modifica el árbol de fuentes ignorado src/ruby-3.1.7. Los archivos
generados obsoletos se mueven a src/ruby-generados-originales, sin borrarlos.
"""

import hashlib
from pathlib import Path
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parent
SRC = ROOT / "src" / "ruby-3.1.7"
TARBALL = ROOT / "src" / "ruby-3.1.7.tar.gz"
EXPECTED_SHA256 = "0556acd69f141ddace03fa5dd8d76e7ea0d8f5232edf012429579bcdaab30e7b"
GENERATED = (
    "parse.c", "id.h", "insns.inc", "insns_info.inc", "opt_sc.inc",
    "optinsn.inc", "optunifs.inc", "vm.inc", "vmtc.inc", "mjit_compile.inc",
)


def main():
    if not TARBALL.is_file() or not SRC.is_dir():
        print("faltan el tarball o el árbol extraído de CRuby", file=sys.stderr)
        return 1
    actual = hashlib.sha256(TARBALL.read_bytes()).hexdigest()
    if actual != EXPECTED_SHA256:
        print(f"SHA-256 de Ruby inesperado: {actual}", file=sys.stderr)
        return 1
    patch = shutil.which("patch")
    if not patch:
        print("falta GNU patch en PATH", file=sys.stderr)
        return 1

    patches = sorted((ROOT / "patches").glob("[0-9][0-9][0-9][0-9]-*.patch"))
    if len(patches) != 45 or {p.name[:4] for p in patches} != {f"{n:04d}" for n in range(1, 43)}:
        print("se esperaban 45 parches Ruby numerados del 0001 al 0042", file=sys.stderr)
        return 1

    for number, patch_file in enumerate(patches, 1):
        early = int(patch_file.name[:4]) <= 5
        cwd, level = (ROOT, 0) if early else (SRC, 1)
        command = [patch, f"-p{level}", "--batch", "--forward",
                   "--no-backup-if-mismatch", "-i", str(patch_file)]
        check = subprocess.run(command + ["--dry-run"], cwd=cwd,
                               capture_output=True, text=True)
        if check.returncode:
            print(f"falló el ensayo {number}/{len(patches)}: {patch_file.name}\n"
                  f"{check.stdout}{check.stderr}", file=sys.stderr)
            return 1
        result = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
        if result.returncode:
            print(f"falló la aplicación {number}/{len(patches)}: {patch_file.name}\n"
                  f"{result.stdout}{result.stderr}", file=sys.stderr)
            return 1
        print(f"{number:02d}/{len(patches)} {patch_file.name}: aplicado (-p{level})")

    backup = ROOT / "src" / "ruby-generados-originales"
    if backup.exists():
        print("ya existe src/ruby-generados-originales; no sobrescribir", file=sys.stderr)
        return 1
    present = [name for name in GENERATED if (SRC / name).is_file()]
    if present:
        backup.mkdir()
        for name in present:
            (SRC / name).rename(backup / name)
            print(f"generado obsoleto apartado: {name}")
    print(f"Ruby preparado: {len(patches)} parches; {len(present)} generados apartados")
    return 0


if __name__ == "__main__":
    sys.exit(main())
