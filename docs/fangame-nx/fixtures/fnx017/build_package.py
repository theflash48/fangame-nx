#!/usr/bin/env python3
"""Build the local FNX-017 synthetic dual fixture; never touches a device."""

from hashlib import sha256
from pathlib import Path
import shutil
import subprocess


SOURCE = Path(__file__).resolve().parent
REPO = SOURCE.parents[3]
OUT = REPO / "local" / "fnx017-dual"
PAYLOAD = OUT / "switch" / "fnx017-dual"
EXPECTED_ELF = "28dea29da5df2884074ec049bdf1dda8f4e5d06f1afa6e421a7cae5aaa79ee57"
EXPECTED_ICON = "775a84b79c4f158b6846215a3edccbb96c28c95c6767de5ff3647939055d030d"


def digest(path: Path) -> str:
    return sha256(path.read_bytes()).hexdigest()


def copy(relative_source: str, relative_dest: str) -> None:
    target = PAYLOAD / relative_dest
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(SOURCE / relative_source, target)


def main() -> None:
    if OUT.exists():
        raise SystemExit(f"Refusing to overwrite {OUT}")
    elf = REPO / "mkxp-z-nx" / "rgss-switch.elf"
    icon = REPO / "local" / "fnx009-interactive" / "work" / "icono.jpg"
    nacptool = Path(r"C:\devkitPro\tools\bin\nacptool.exe")
    elf2nro = Path(r"C:\devkitPro\tools\bin\elf2nro.exe")
    if digest(elf) != EXPECTED_ELF or digest(icon) != EXPECTED_ICON:
        raise SystemExit("ELF or original neutral icon hash differs from M016")
    for tool in (nacptool, elf2nro):
        if not tool.is_file():
            raise SystemExit(f"Missing existing tool {tool}")

    for folder in ("Audio", "Graphics", "Data", "lib", "fixtures/a", "fixtures/b"):
        (PAYLOAD / folder).mkdir(parents=True, exist_ok=True)
    for name in ("selector.rb", "mkxp.json", "Game.ini", "siguiente.txt"):
        copy(name, name)
    copy("scene_a.rb", "fixtures/a/scene.rb")
    copy("scene_b.rb", "fixtures/b/scene.rb")
    copy("entrada_a.txt", "fixtures/a/entrada.txt")
    copy("entrada_b.txt", "fixtures/b/entrada.txt")
    shutil.copyfile(REPO / "runtime/lib/nx-compat.rb", PAYLOAD / "lib/nx-compat.rb")
    shutil.copyfile(REPO / "runtime/lib/socket.rb", PAYLOAD / "lib/socket.rb")
    (PAYLOAD / "Data/Scripts.rxdata").write_bytes(b"")

    work = OUT / "work"
    work.mkdir()
    nacp = work / "fnx017.nacp"
    nro = PAYLOAD / "fnx017-dual.nro"
    subprocess.run(
        [str(nacptool), "--create", "FangameNX FNX-017", "FangameNX", "0.0.4", str(nacp)],
        check=True,
    )
    subprocess.run(
        [
            str(elf2nro), str(elf), str(nro), f"--icon={icon}",
            f"--nacp={nacp}", f"--romfsdir={REPO / 'mkxp-z-nx/romfs'}",
        ],
        check=True,
    )

    files = sorted(path for path in PAYLOAD.rglob("*") if path.is_file())
    if len(files) != 12 or sum(path.suffix == ".nro" for path in files) != 1:
        raise SystemExit("Unexpected package inventory")
    manifest = OUT / "MANIFEST.sha256.txt"
    manifest.write_text(
        "".join(f"{digest(path)}  {path.relative_to(PAYLOAD).as_posix()}\n" for path in files),
        encoding="utf-8",
    )
    print(f"PACKAGE={PAYLOAD}")
    print(f"ELF={EXPECTED_ELF}")
    print(f"NRO={digest(nro)}")
    print(f"FILES={len(files)}")
    print(f"MANIFEST={digest(manifest)}")


if __name__ == "__main__":
    main()
