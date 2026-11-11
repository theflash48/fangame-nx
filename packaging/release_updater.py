#!/usr/bin/env python3
"""Consulta y prepara releases de FangameNX en PC. Nunca instala en Switch."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import tempfile
import urllib.request
import urllib.error
import zipfile

PACKAGE_ID = "fangamenx-runtime"
DESTINATION = "switch/FangameNX/FangameNX.nro"
MAX_PACKAGE = 64 * 1024 * 1024
MAX_METADATA = 2 * 1024 * 1024
HEX256 = re.compile(r"^[0-9a-f]{64}$")
REPO = re.compile(r"^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$")
VERSION = re.compile(r"^v?(\d+)\.(\d+)\.(\d+)(?:[-+]b?(\d+))?$", re.I)


def version_key(value):
    match = VERSION.fullmatch(value)
    if not match:
        raise ValueError("Versión incompatible: " + value)
    return tuple(int(item or 0) for item in match.groups())


def digest(path):
    sha = hashlib.sha256()
    with open(path, "rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            sha.update(block)
    return sha.hexdigest()


def validate_manifest(manifest):
    if not isinstance(manifest, dict) or manifest.get("protocol") != 1 or \
       manifest.get("id") != PACKAGE_ID or manifest.get("platform") != "switch" or \
       manifest.get("arch") != "aarch64" or manifest.get("channel") not in ("stable", "beta"):
        raise ValueError("Manifiesto incompatible")
    version_key(manifest.get("version", ""))
    files = manifest.get("files")
    if not isinstance(files, list) or len(files) != 1:
        raise ValueError("Lista de archivos incompatible")
    item = files[0]
    if not isinstance(item, dict) or item.get("path") != DESTINATION or \
       type(item.get("size")) is not int or not 1024 <= item["size"] <= MAX_PACKAGE or \
       not isinstance(item.get("sha256"), str) or not HEX256.fullmatch(item["sha256"]):
        raise ValueError("Destino, tamaño o hash inválido")
    return item


def package(nro, output, version, channel):
    nro, output = Path(nro), Path(output)
    if nro.stat().st_size < 1024 or nro.stat().st_size > MAX_PACKAGE:
        raise ValueError("Tamaño NRO inválido")
    with nro.open("rb") as stream:
        stream.seek(16)
        if stream.read(4) != b"NRO0":
            raise ValueError("Cabecera NRO inválida")
    manifest = {"protocol": 1, "id": PACKAGE_ID, "platform": "switch",
                "arch": "aarch64", "channel": channel, "version": version,
                "files": [{"path": DESTINATION, "size": nro.stat().st_size,
                           "sha256": digest(nro)}]}
    validate_manifest(manifest)
    output.parent.mkdir(parents=True, exist_ok=True)
    temp = output.with_name(output.name + ".tmp")
    with zipfile.ZipFile(temp, "w", compression=zipfile.ZIP_STORED) as archive:
        for name, data in (("manifest.json", json.dumps(manifest, sort_keys=True,
                              separators=(",", ":")).encode() + b"\n"),
                           (DESTINATION, nro.read_bytes())):
            info = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_STORED
            info.external_attr = 0o644 << 16
            archive.writestr(info, data)
    os.replace(temp, output)
    verify_package(output)
    return manifest


def verify_package(path, expected_digest=None):
    path = Path(path)
    if path.stat().st_size > MAX_PACKAGE:
        raise ValueError("Paquete demasiado grande")
    if expected_digest is not None:
        if not HEX256.fullmatch(expected_digest) or digest(path) != expected_digest:
            raise ValueError("Hash del asset no coincide")
    with zipfile.ZipFile(path) as archive:
        if set(archive.namelist()) != {"manifest.json", DESTINATION} or \
           len(archive.namelist()) != 2:
            raise ValueError("Contenido no permitido")
        if archive.getinfo("manifest.json").file_size > 16384:
            raise ValueError("Manifiesto demasiado grande")
        manifest = json.loads(archive.read("manifest.json"))
        item = validate_manifest(manifest)
        entry = archive.getinfo(DESTINATION)
        if entry.file_size != item["size"] or entry.file_size > MAX_PACKAGE:
            raise ValueError("Tamaño interno inválido")
        sha = hashlib.sha256()
        with archive.open(entry) as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""):
                sha.update(block)
        if sha.hexdigest() != item["sha256"]:
            raise ValueError("Hash interno no coincide")
    return manifest


def release_url(repo, channel):
    if not REPO.fullmatch(repo) or ".." in repo:
        raise ValueError("Repositorio inválido")
    suffix = "/releases/latest" if channel == "stable" else "/releases?per_page=30"
    return "https://api.github.com/repos/" + repo + suffix


def select_release(data, channel, current):
    releases = data if isinstance(data, list) else [data]
    candidates = []
    for release in releases:
        if not isinstance(release, dict) or release.get("draft"):
            continue
        if channel == "stable" and release.get("prerelease"):
            continue
        try:
            version = version_key(release["tag_name"])
        except (KeyError, ValueError, TypeError):
            continue
        if version <= version_key(current):
            continue
        assets = [asset for asset in release.get("assets", [])
                  if isinstance(asset, dict) and
                  re.fullmatch(r"FangameNX-[A-Za-z0-9.+-]+\.zip", asset.get("name", "")) and
                  type(asset.get("size")) is int and 1024 <= asset["size"] <= MAX_PACKAGE]
        if assets:
            candidates.append((version, release, assets[0]))
    if not candidates:
        return None
    _, release, asset = max(candidates, key=lambda entry: entry[0])
    raw_digest = asset.get("digest", "")
    value = raw_digest[7:] if raw_digest.startswith("sha256:") else None
    if value is not None and not HEX256.fullmatch(value):
        raise ValueError("Digest de GitHub inválido")
    return {"tag": release["tag_name"], "published_at": release.get("published_at"),
            "notes": str(release.get("body") or "")[:10000],
            "asset": asset["name"], "size": asset["size"],
            "sha256": value, "url": asset.get("browser_download_url")}


def check(repo, channel, current):
    request = urllib.request.Request(release_url(repo, channel), headers={
        "Accept": "application/vnd.github+json", "User-Agent": "FangameNX-updater/1",
        "X-GitHub-Api-Version": "2026-03-10"})
    try:
        with urllib.request.urlopen(request, timeout=15) as response:
            if response.status != 200 or int(response.headers.get("Content-Length", 0)) > MAX_METADATA:
                raise ValueError("Respuesta GitHub inválida")
            raw = response.read(MAX_METADATA + 1)
    except urllib.error.HTTPError as error:
        if error.code == 404:
            return None
        raise ValueError(f"GitHub API respondió {error.code}") from error
    if len(raw) > MAX_METADATA:
        raise ValueError("Metadatos demasiado grandes")
    return select_release(json.loads(raw), channel, current)


def stage(release, repo, destination):
    release_url(repo, "stable")
    url = release.get("url")
    prefix = "https://github.com/" + repo + "/releases/download/"
    if not isinstance(url, str) or not url.startswith(prefix) or not release.get("sha256") or \
       not re.fullmatch(r"FangameNX-[A-Za-z0-9.+-]+\.zip", str(release.get("asset", ""))):
        raise ValueError("Asset sin URL o digest verificable")
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    if shutil.disk_usage(destination).free < release["size"] * 2:
        raise ValueError("Espacio insuficiente para staging")
    fd, temp = tempfile.mkstemp(prefix="fangamenx-", suffix=".part", dir=destination)
    os.close(fd)
    try:
        request = urllib.request.Request(url, headers={"User-Agent": "FangameNX-updater/1"})
        with urllib.request.urlopen(request, timeout=30) as response, open(temp, "wb") as target:
            if response.status != 200:
                raise ValueError("Descarga fallida")
            count = 0
            while chunk := response.read(1024 * 1024):
                count += len(chunk)
                if count > MAX_PACKAGE or count > release["size"]:
                    raise ValueError("Descarga demasiado grande")
                target.write(chunk)
            target.flush(); os.fsync(target.fileno())
        if count != release["size"]:
            raise ValueError("Descarga incompleta")
        verify_package(temp, release["sha256"])
        final = destination / release["asset"]
        os.replace(temp, final)
        return final
    finally:
        if os.path.exists(temp):
            os.unlink(temp)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    make = sub.add_parser("package")
    make.add_argument("nro"); make.add_argument("output")
    make.add_argument("--version", required=True)
    make.add_argument("--channel", choices=("stable", "beta"), default="beta")
    verify = sub.add_parser("verify")
    verify.add_argument("package"); verify.add_argument("--sha256")
    query = sub.add_parser("check")
    query.add_argument("--repo", required=True)
    query.add_argument("--channel", choices=("stable", "beta"), default="stable")
    query.add_argument("--current", required=True)
    args = parser.parse_args()
    if args.command == "package":
        result = package(args.nro, args.output, args.version, args.channel)
    elif args.command == "verify":
        result = verify_package(args.package, args.sha256)
    else:
        result = check(args.repo, args.channel, args.current)
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
