"""Checks for the metadata-only public NRO and its NSP forwarder inputs."""

import importlib.util
import struct
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def load_tool(name, filename):
    spec = importlib.util.spec_from_file_location(name, ROOT / "tools" / "build" / filename)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


public_nro = load_tool("public_nro", "prepare-public-nro.py")
forwarder = load_tool("forwarder", "make-forwarder.py")


class PublicPackageTests(unittest.TestCase):
    def setUp(self):
        self.icon = (
            b"\xff\xd8\xff\xe1\x00\x06EXIF"
            b"\xff\xc0\x00\x11\x08\x01\x00\x01\x00\x03"
            b"\x01\x11\x00\x02\x11\x01\x03\x11\x01"
            b"\xff\xda\x00\x02\x11\x22\xff\xd9"
        )
        self.nacp = bytes(public_nro.NACP_SIZE)
        self.executable = bytearray(0x80)
        self.executable[0x10:0x14] = b"NRO0"
        struct.pack_into("<I", self.executable, 0x18, len(self.executable))

    def test_icon_strips_exif_and_checks_size(self):
        clean = public_nro.clean_icon(self.icon)
        self.assertNotIn(b"EXIF", clean)
        self.assertTrue(clean.endswith(b"\xff\xd9"))
        with self.assertRaisesRegex(ValueError, "256x256"):
            public_nro.clean_icon(self.icon.replace(b"\x01\x00\x01\x00", b"\x01\x00\x01\x01"))

    def test_code_bytes_unchanged_and_metadata_updated(self):
        icon = public_nro.clean_icon(self.icon)
        original = bytes(self.executable) + public_nro.ASSET_HEADER.pack(
            b"ASET", 0, 56, len(icon), 56 + len(icon), len(self.nacp), 0, 0
        ) + icon + self.nacp
        result, code_size = public_nro.repackage(original, icon, "DS2", "xDASCATx", "0.56")
        self.assertEqual(result[:code_size], original[:code_size])
        extracted_icon, nacp = forwarder.nro_assets(result)
        self.assertEqual(extracted_icon, icon)
        self.assertEqual(nacp[0x3060:0x3070].split(b"\0")[0], b"0.56")
        self.assertEqual(nacp[0xF00:0xF03], b"DS2")
        self.assertEqual(nacp[0x1100:0x1109], b"xDASCATx\0")

    def test_rejects_unexpected_romfs(self):
        original = bytes(self.executable) + public_nro.ASSET_HEADER.pack(
            b"ASET", 0, 56, len(self.icon), 56 + len(self.icon), len(self.nacp), 56, 1
        ) + self.icon + self.nacp
        with self.assertRaisesRegex(ValueError, "ROMFS"):
            public_nro.repackage(original, self.icon, "DS2", "xDASCATx", "0.56")


if __name__ == "__main__":
    unittest.main()
