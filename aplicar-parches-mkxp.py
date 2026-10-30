#!/usr/bin/env python3
"""Aplica el conjunto versionado de parches a src/mkxp-z 2.4.2.

El árbol de trabajo y los logs viven únicamente bajo src/, ignorado por Git.
No borra ni sustituye la fuente: se detiene si un parche no aplica limpio.
"""

import hashlib
import os
from pathlib import Path
import runpy
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parent
SRC = ROOT / "src" / "mkxp-z"
TARBALL = ROOT / "src" / "mkxpz.tar.gz"
ORDER = ROOT / "patches" / "ORDEN.txt"
EXPECTED_SHA256 = "4c2df25e30139678307a8d072c325dc94c01ebc024c47d51eee17dd6f2ee1aac"


def main():
    if not TARBALL.is_file() or not SRC.is_dir() or not ORDER.is_file():
        print("faltan el tarball, el árbol extraído o patches/ORDEN.txt", file=sys.stderr)
        return 1
    actual = hashlib.sha256(TARBALL.read_bytes()).hexdigest()
    if actual != EXPECTED_SHA256:
        print(f"SHA-256 de mkxpz.tar.gz inesperado: {actual}", file=sys.stderr)
        return 1
    patch = shutil.which("patch")
    if not patch:
        print("falta GNU patch en PATH", file=sys.stderr)
        return 1

    names = [line.strip() for line in ORDER.read_text(encoding="utf-8").splitlines()
             if line.strip() and not line.startswith("#")]
    expected = {path.name for path in (ROOT / "patches").glob("mkxpz-*.patch")}
    if len(names) != len(set(names)) or set(names) != expected:
        print("ORDEN.txt no coincide con los parches mkxp-z versionados", file=sys.stderr)
        return 1

    resolver = runpy.run_path(str(ROOT / "reconstruir-src.py"))["donde_aplicar"]
    for number, name in enumerate(names, 1):
        patch_file = ROOT / "patches" / name
        cwd, level = resolver(str(SRC), str(patch_file))
        if not cwd or not Path(cwd).resolve().is_relative_to(SRC.resolve()):
            print(f"no se puede ubicar {name} dentro de src/mkxp-z", file=sys.stderr)
            return 1
        command = [patch, f"-p{level}", "--batch", "--forward",
                   "--no-backup-if-mismatch", "-i", str(patch_file)]
        check = subprocess.run(command + ["--dry-run"], cwd=cwd, capture_output=True, text=True)
        if check.returncode:
            print(f"falló el ensayo {number}/{len(names)}: {name}\n{check.stdout}{check.stderr}",
                  file=sys.stderr)
            return 1
        result = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
        if result.returncode:
            print(f"falló la aplicación {number}/{len(names)}: {name}\n{result.stdout}{result.stderr}",
                  file=sys.stderr)
            return 1
        print(f"{number:02d}/{len(names)} {name}: aplicado (-p{level})")

    temp = ROOT / "src" / "temp"
    temp.mkdir(exist_ok=True)
    env = dict(os.environ, TMP=str(temp), TEMP=str(temp), TMPDIR=str(temp))
    verify = subprocess.run([sys.executable, str(ROOT / "reconstruir-src.py")],
                            cwd=ROOT, env=env)
    return verify.returncode


if __name__ == "__main__":
    sys.exit(main())
