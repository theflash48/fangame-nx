"""Pruebas locales del formato de actualización, sin acceso a Switch ni red."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

source = Path(__file__).resolve().parents[1] / "packaging/release_updater.py"
spec = importlib.util.spec_from_file_location("release_updater", source)
updater = importlib.util.module_from_spec(spec)
spec.loader.exec_module(updater)


class ReleaseTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.nro = self.root / "FangameNX.nro"
        self.nro.write_bytes(bytes(16) + b"NRO0" + bytes(2028))
        self.archive = self.root / "FangameNX-0.2.5-b39.zip"
        updater.package(self.nro, self.archive, "0.2.5-b39", "beta")

    def release(self, tag="v0.2.5-b39", beta=False, assets=True):
        return {"tag_name": tag, "draft": False, "prerelease": beta,
                "published_at": "2026-11-11T00:00:00Z", "body": "Notas",
                "assets": [{"name": self.archive.name, "size": self.archive.stat().st_size,
                            "digest": "sha256:" + updater.digest(self.archive),
                            "browser_download_url":
                            "https://github.com/example/fangame-nx/releases/download/v0.2.5-b39/" + self.archive.name}]
                if assets else []}

    def test_version_channels_and_missing_asset(self):
        self.assertIsNone(updater.select_release([self.release("v0.2.4-b38")], "beta", "0.2.4-b38"))
        self.assertIsNone(updater.select_release([self.release(beta=True)], "stable", "0.2.4-b38"))
        self.assertIsNone(updater.select_release([self.release(assets=False)], "beta", "0.2.4-b38"))
        self.assertEqual(updater.select_release([self.release(beta=True)], "beta", "0.2.4-b38")["tag"],
                         "v0.2.5-b39")

    def test_package_rejects_hash_content_and_destination(self):
        manifest = updater.verify_package(self.archive, updater.digest(self.archive))
        self.assertEqual(manifest["files"][0]["path"], updater.DESTINATION)
        with self.assertRaises(ValueError):
            updater.verify_package(self.archive, "0" * 64)
        bad = self.root / "bad.zip"
        with zipfile.ZipFile(self.archive) as original, zipfile.ZipFile(bad, "w") as output:
            output.writestr("manifest.json", original.read("manifest.json"))
            output.writestr(updater.DESTINATION, b"x" * self.nro.stat().st_size)
        with self.assertRaisesRegex(ValueError, "Hash interno"):
            updater.verify_package(bad)
        manifest["files"][0]["path"] = "switch/FangameNX/games/pokemon-z/save.dat"
        with self.assertRaises(ValueError):
            updater.validate_manifest(manifest)

    def test_rejects_untrusted_source(self):
        with self.assertRaises(ValueError):
            updater.release_url("../bad", "stable")
        release = updater.select_release([self.release()], "beta", "0.2.4-b38")
        release["url"] = "https://example.com/file.zip"
        with self.assertRaisesRegex(ValueError, "URL"):
            updater.stage(release, "example/fangame-nx", self.root / "staging")

    def test_incomplete_download_leaves_no_staged_package(self):
        release = updater.select_release([self.release()], "beta", "0.2.4-b38")
        class ShortResponse:
            status = 200
            def __enter__(self): return self
            def __exit__(self, *_): pass
            def read(self, _): return b""
        staging = self.root / "staging"
        with patch.object(updater.urllib.request, "urlopen", return_value=ShortResponse()):
            with self.assertRaisesRegex(ValueError, "incompleta"):
                updater.stage(release, "example/fangame-nx", staging)
        self.assertEqual(list(staging.iterdir()), [])


if __name__ == "__main__":
    unittest.main()
