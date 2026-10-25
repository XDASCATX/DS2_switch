import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib


class ShaderLibraryExtractionTests(unittest.TestCase):
    def test_physical_offset_and_padding(self):
        code = struct.pack(">III", 1, 0x2000, 0)
        container = bytearray(0x70)
        struct.pack_into(">III", container, 0, 0x102A1100, 0x50, 0x20)
        struct.pack_into(">I", container, 0x18, 0x24)
        struct.pack_into(">II", container, 0x24, 8, len(code))
        container[0x58:0x58 + len(code)] = code
        compressed = zlib.compress(container)
        archive = bytearray(0x90 + len(compressed))
        archive[:4] = b"BND4"
        archive[9] = 1
        struct.pack_into(">i", archive, 0x0C, 1)
        struct.pack_into(">q", archive, 0x20, 0x24)
        struct.pack_into(">qqIII", archive, 0x48, len(compressed), len(container), 0x90, 0, 0x70)
        archive[0x70:0x79] = b"test.fpo\0"
        archive[0x90:] = compressed
        script = Path(__file__).resolve().parents[1] / "tools" / "shaders" / "extract_library.py"
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "game" / "shader").mkdir(parents=True)
            (root / "game" / "shader" / "test.bnd").write_bytes(archive)
            result = subprocess.run([sys.executable, "-I", str(script), str(root / "game"), str(root / "output")],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            sources = json.loads((root / "output" / "sources.json").read_text())
            self.assertEqual(len(sources), 1)
            self.assertEqual(sources[0]["stage"], "ps")
            self.assertEqual((root / "output" / sources[0]["file"]).read_bytes(), code)


if __name__ == "__main__":
    unittest.main()
