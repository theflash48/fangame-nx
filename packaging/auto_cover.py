#!/usr/bin/env python3
"""Elige una portada conservadora de una instalación preparada para FangameNX."""
# SPDX-License-Identifier: GPL-2.0-or-later
import json
from pathlib import Path
import re
import struct
import zipfile
import zlib

MAX_BYTES = 8 * 1024 * 1024
MAX_SIDE = 2048
NAMES = {"titulo": 0, "title": 0, "portada": 1, "cover": 1,
         "titlelogo": 2, "logotitulo": 2, "logo": 3}


def priority(name: str) -> int | None:
    parts = name.replace("\\", "/").split("/")
    if len(parts) < 3 or parts[0].lower() != "graphics":
        return None
    if not parts[-1].lower().endswith(".png"):
        return None
    stem = Path(parts[-1]).stem.casefold().replace("_", "").replace("-", "")
    folder = parts[1].casefold()
    if stem not in NAMES:
        return None
    if folder == "pictures":
        return NAMES[stem]
    if folder in {"titles", "titles1", "titles2", "title"}:
        return 10 + NAMES[stem]
    return None


def valid_png(data: bytes) -> bool:
    if len(data) < 45 or len(data) > MAX_BYTES or data[:8] != b"\x89PNG\r\n\x1a\n":
        return False
    pos = 8
    first = True
    image_data = False
    while pos + 12 <= len(data):
        size = struct.unpack_from(">I", data, pos)[0]
        end = pos + 12 + size
        if end > len(data):
            return False
        kind = data[pos + 4:pos + 8]
        payload = data[pos + 8:pos + 8 + size]
        crc = struct.unpack_from(">I", data, pos + 8 + size)[0]
        if zlib.crc32(kind + payload) != crc:
            return False
        if first:
            if kind != b"IHDR" or size != 13:
                return False
            width, height = struct.unpack_from(">II", payload)
            if not (128 <= width <= MAX_SIDE and 128 <= height <= MAX_SIDE):
                return False
            first = False
        if kind == b"IDAT":
            image_data = True
        if kind == b"IEND":
            return image_data and size == 0 and end == len(data)
        pos = end
    return False


def choose(game: Path) -> tuple[str, bytes] | None:
    candidates: list[tuple[int, str, bytes]] = []
    graphics = game / "Graphics"
    if graphics.is_dir():
        for folder in ("Pictures", "Titles", "Titles1", "Titles2", "Title"):
            directory = graphics / folder
            if not directory.is_dir():
                continue
            for path in directory.iterdir():
                rank = priority(path.relative_to(game).as_posix())
                if rank is not None and path.is_file() and path.stat().st_size <= MAX_BYTES:
                    data = path.read_bytes()
                    if valid_png(data):
                        candidates.append((rank, path.relative_to(game).as_posix(), data))
    archive = game / "datos.zip"
    if archive.is_file():
        with zipfile.ZipFile(archive) as z:
            for entry in z.infolist():
                rank = priority(entry.filename)
                if rank is not None and not entry.is_dir() and entry.file_size <= MAX_BYTES:
                    data = z.read(entry)
                    if valid_png(data):
                        candidates.append((rank, entry.filename, data))
    if not candidates:
        return None
    candidates.sort(key=lambda item: (item[0], item[1].casefold()))
    best = [item for item in candidates if item[0] == candidates[0][0]]
    if len(best) != 1:
        return None
    return best[0][1], best[0][2]


def install_cover(game: Path, game_id: str | None = None) -> str:
    manifest = game / "fangamenx.json"
    metadata = json.loads(manifest.read_text(encoding="utf-8")) if manifest.is_file() else {}
    if not isinstance(metadata, dict):
        raise ValueError("fangamenx.json debe ser un objeto JSON")
    if "cover" in metadata and metadata["cover"] != "":
        named = metadata["cover"]
        if (not isinstance(named, str) or len(named) > 80 or named.startswith(".") or
                ".." in named or not re.fullmatch(r"[A-Za-z0-9_.-]+\.(?:png|jpg|jpeg)", named)):
            raise ValueError("campo cover no válido")
        explicit = game / named
        if not explicit.is_file() or explicit.stat().st_size > MAX_BYTES:
            return "portada explícita ausente o demasiado grande; se usará la reserva"
        if named.endswith(".png") and not valid_png(explicit.read_bytes()):
            return "portada PNG explícita inválida; se usará la reserva"
        return "portada explícita conservada"
    cover = game / "cover.png"
    if cover.is_file():
        if cover.stat().st_size > MAX_BYTES or not valid_png(cover.read_bytes()):
            return "cover.png no es una imagen PNG válida para el lanzador"
        source = "cover.png existente"
    else:
        selected = choose(game)
        if selected is None:
            return "sin imagen de título única y válida; se usará la portada de reserva"
        source, data = selected
        cover.write_bytes(data)
    metadata.setdefault("id", game_id or game.name)
    metadata["cover"] = "cover.png"
    manifest.write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return f"cover.png desde {source}"
