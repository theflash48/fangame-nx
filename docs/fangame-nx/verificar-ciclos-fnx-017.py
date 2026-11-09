#!/usr/bin/env python3
"""Compare before/after copies of the four FNX009 logs without touching MTP."""

import argparse
from pathlib import Path
import re
import sys


FILES = (
    "fnx009-probe.log",
    "fnx009-escritura.txt",
    "mkxp-boot.log",
    "mkxp-debug.log",
)
RUBY_START = "=== FNX009 INICIO ==="
BOOT_START = re.compile(r"^\[\s*\d+\] \[build\] ")


def delta(before_dir: Path, after_dir: Path, name: str) -> str:
    before = (before_dir / name).read_bytes()
    after = (after_dir / name).read_bytes()
    if not after.startswith(before):
        raise ValueError(f"{name}: el archivo previo no es prefijo del posterior")
    return after[len(before) :].decode("utf-8")


def blocks(text: str, is_start) -> list[list[str]]:
    result = []
    current = None
    for line in text.splitlines():
        if is_start(line):
            current = []
            result.append(current)
        if current is None:
            if line.strip():
                raise ValueError(f"texto sin marca inicial: {line[:100]}")
            continue
        current.append(line)
    return result


def require_lines(block: list[str], needles: tuple[str, ...], label: str) -> None:
    body = "\n".join(block)
    for needle in needles:
        if needle not in body:
            raise ValueError(f"{label}: falta {needle!r}")


def verify(before: Path, after: Path, expected: int) -> None:
    for name in FILES:
        if not (before / name).is_file() or not (after / name).is_file():
            raise ValueError(f"falta copia previa o posterior de {name}")

    ruby = blocks(delta(before, after, "fnx009-probe.log"), lambda x: x == RUBY_START)
    boot = blocks(delta(before, after, "mkxp-boot.log"), lambda x: bool(BOOT_START.match(x)))
    debug = blocks(
        delta(before, after, "mkxp-debug.log"),
        lambda x: x.startswith("[nx] mkxp-z arrancando"),
    )
    writing = delta(before, after, "fnx009-escritura.txt")

    counts = (len(ruby), len(boot), len(debug))
    if counts != (expected, expected, expected):
        raise ValueError(f"bloques Ruby/boot/debug={counts}, esperados={expected}")
    if writing != "FNX009-ESCRITURA-OK\n" * expected:
        raise ValueError("marcadores de escritura nuevos incompletos o inesperados")

    for i, (r, b, d) in enumerate(zip(ruby, boot, debug), 1):
        require_lines(
            r,
            (
                "lectura=OK",
                "escritura=OK",
                "boton=A contador=1",
                "boton=X color=verde",
                "boton=B salida_solicitada",
                "FNX009 RUBY COMPLETADO; salida normal",
            ),
            f"Ruby ciclo {i}",
        )
        require_lines(
            b,
            (
                "[nro] sdmc:/switch/fnx016-sdl-fix/fnx016-sdl-fix.nro",
                "[exit] RGSS after GL unbind",
                "[exit] main after GL delete",
                "[exit] 03 SDL_DestroyWindow OK",
                "[exit] 07 SDL_Quit OK",
                "[exit] 08 before return main",
            ),
            f"boot ciclo {i}",
        )
        require_lines(d, ("EventThread termination requested", "Shutting down."), f"debug ciclo {i}")

    print(f"OK: {expected} ciclo(s) con logs completos y prefijos conservados")
    print("La vuelta visible al Homebrew Menu requiere observacion del propietario en cada ciclo.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("before", type=Path, help="copias locales antes de la serie")
    parser.add_argument("after", type=Path, help="copias locales despues de la serie")
    parser.add_argument("--expected", type=int, default=5)
    args = parser.parse_args()
    if args.expected < 1:
        parser.error("--expected debe ser positivo")
    try:
        verify(args.before, args.after, args.expected)
    except (OSError, UnicodeError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
