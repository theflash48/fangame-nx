#!/usr/bin/env python3
"""Regresión local del preparador con datos sintéticos, sin fangames."""
# SPDX-License-Identifier: GPL-2.0-or-later
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib
import zipfile

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "packaging/prepare-launcher-game.py"
INSTALL = ROOT / "packaging/install-launcher-game.py"
sys.path.insert(0, str(ROOT / "packaging"))
from auto_cover import install_cover, valid_png


def png(width: int = 128, height: int = 128) -> bytes:
    def chunk(name: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + name + data + struct.pack(">I", zlib.crc32(name + data))
    rows = b"".join(b"\0" + b"\0\0\0\xff" * width for _ in range(height))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


def run(*args: object) -> subprocess.CompletedProcess:
    return subprocess.run([sys.executable, str(TOOL), *(str(a) for a in args)],
                          text=True, capture_output=True, cwd=ROOT)


def main() -> None:
    assert valid_png(png())
    assert not valid_png(png()[:-1])
    assert not valid_png(png(2049, 128))
    (ROOT / "local").mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=ROOT / "local") as temp:
        base = Path(temp)
        game = base / "input"
        (game / "Data").mkdir(parents=True)
        (game / "Graphics/Pictures").mkdir(parents=True)
        (game / "Audio").mkdir()
        (game / "Game.ini").write_text("[Game]\nTitle=Juego de prueba\nLibrary=RGSS104E.dll\nScripts=Data\\Scripts.rxdata\n")
        (game / "Data/Scripts.rxdata").write_bytes(b"datos sinteticos")
        (game / "Graphics/Pictures/titulo.png").write_bytes(png())
        (game / "Audio/sonido.ogg").write_bytes(b"sonido sintetico")
        source_hash = hashlib.sha256((game / "Graphics/Pictures/titulo.png").read_bytes()).hexdigest()
        invalid_cover = base / "invalid-cover"
        invalid_cover.mkdir()
        (invalid_cover / "cover.png").write_bytes(png()[:-1])
        assert "no es una imagen PNG válida" in install_cover(invalid_cover)
        assert not (invalid_cover / "fangamenx.json").exists()

        check = run(game, "--diagnose", "--family", "rgss1", "--id", "juego-prueba")
        assert check.returncode == 0, check.stderr
        assert json.loads(check.stdout)["ready_for_staging"]
        assert not (base / "stage").exists()

        stage = base / "stage"
        created = run(game, "--output", stage, "--family", "rgss1", "--id", "juego-prueba")
        assert created.returncode == 0, created.stderr
        assert stage.is_dir() and (stage / "Game.rgssad").read_bytes() == b"RGSSAD\0\1"
        assert hashlib.sha256((stage / "cover.png").read_bytes()).hexdigest() == source_hash
        with zipfile.ZipFile(stage / "datos.zip") as archive:
            assert archive.testzip() is None
            assert set(archive.namelist()) == {"Data/Scripts.rxdata", "Graphics/Pictures/titulo.png", "Audio/sonido.ogg"}
            assert all(item.compress_type == zipfile.ZIP_STORED for item in archive.infolist())
            assert all(not item.extra for item in archive.infolist())
        config = json.loads((stage / "mkxp.json").read_text(encoding="utf-8"))
        assert "lib/nx-ruby18.rb" in config["preloadScript"]
        assert json.loads((stage / "fangamenx.json").read_text(encoding="utf-8"))["cover"] == "cover.png"
        manifest = json.loads((base / "stage.manifest.json").read_text(encoding="utf-8"))
        for item in manifest["files"]:
            file = stage / item["path"]
            assert file.stat().st_size == item["size"]
            assert hashlib.sha256(file.read_bytes()).hexdigest() == item["sha256"]
        assert len(manifest["files"]) == len([p for p in stage.rglob("*") if p.is_file()])

        repeated = run(game, "--output", stage, "--family", "rgss1", "--id", "juego-prueba")
        assert repeated.returncode == 0 and '"reused": true' in repeated.stdout.lower(), repeated.stderr
        zipped = base / "juego.zip"
        with zipfile.ZipFile(zipped, "w") as archive:
            for file in game.rglob("*"):
                if file.is_file():
                    archive.write(file, "Juego/" + file.relative_to(game).as_posix())
        zip_check = run(zipped, "--diagnose", "--family", "rgss1", "--id", "juego-zip")
        assert zip_check.returncode == 0 and json.loads(zip_check.stdout)["ready_for_staging"]
        zip_stage = base / "zipstage"
        zip_build = run(zipped, "--output", zip_stage, "--family", "rgss1", "--id", "juego-zip")
        assert zip_build.returncode == 0, zip_build.stderr
        assert (zip_stage / "cover.png").read_bytes() == png()
        library = base / "games"
        library.mkdir()
        installed = subprocess.run([sys.executable, str(INSTALL), str(stage), str(library)],
                                   text=True, capture_output=True, cwd=ROOT)
        assert installed.returncode == 0, installed.stderr
        assert hashlib.sha256((library / "juego-prueba/cover.png").read_bytes()).hexdigest() == source_hash
        (game / "Audio/sonido.ogg").write_bytes(b"fuente modificada")
        conflict = run(game, "--output", stage, "--family", "rgss1", "--id", "juego-prueba")
        assert conflict.returncode == 2 and "otra fuente" in conflict.stderr

        bad = base / "unsafe.zip"
        with zipfile.ZipFile(bad, "w") as archive:
            archive.writestr("../escape", b"no")
        rejected = run(bad, "--diagnose")
        assert rejected.returncode == 2 and "insegura" in rejected.stderr
        collision = base / "collision.zip"
        with zipfile.ZipFile(collision, "w") as archive:
            archive.writestr("Game.ini", b"[Game]\nTitle=Uno")
            archive.writestr("game.ini", b"[Game]\nTitle=Dos")
        rejected = run(collision, "--diagnose")
        assert rejected.returncode == 2 and "duplicada" in rejected.stderr
        incomplete = base / "incomplete"
        (incomplete / "Data").mkdir(parents=True)
        (incomplete / "Graphics").mkdir()
        (incomplete / "Game.ini").write_text("[Game]\nTitle=Incompleto\n")
        (incomplete / "Data/Scripts.rxdata").write_bytes(b"script")
        (incomplete / "Graphics/title.png").write_bytes(png())
        missing = run(incomplete, "--diagnose", "--family", "rgss1")
        assert missing.returncode == 2 and "Falta Audio/" in missing.stdout
        directory_profile = base / "directory-profile"
        (directory_profile / "Data/Scripts").mkdir(parents=True)
        (directory_profile / "Plugins").mkdir()
        (directory_profile / "Graphics").mkdir()
        (directory_profile / "Audio").mkdir()
        (directory_profile / "Game.ini").write_text("[Game]\nTitle=Pokemon Vanguard\n")
        (directory_profile / "Data/Scripts/main.rb").write_text("# sintético\n")
        (directory_profile / "Plugins/plugin.rb").write_text("# sintético\n")
        (directory_profile / "Graphics/logo.png").write_bytes(png())
        (directory_profile / "Audio/silencio.ogg").write_bytes(b"sonido")
        found = run(directory_profile, "--diagnose")
        assert found.returncode == 0, found.stderr
        assert json.loads(found.stdout)["profile"] == "pokemon-vanguard"
        assert (game / "Graphics/Pictures/titulo.png").is_file()
    print("prepare_launcher_game: OK")


if __name__ == "__main__":
    main()
