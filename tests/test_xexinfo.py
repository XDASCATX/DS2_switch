import os, struct, sys, tempfile, unittest
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tools'))
import ds2_xexinfo as x


def build_xex(title_id=0x4E4D0925, compression=2, encryption=1, magic=b'XEX2'):
    be = lambda *v: struct.pack('>' + 'I' * len(v), *v)
    ff = be(8) + struct.pack('>HH', encryption, compression)
    ex = be(0xAABBCCDD, 0x1, 0x1, title_id) + bytes([1, 2, 1, 1]) + be(0)
    pe = b'\0\0\0\x10' + b'default.exe\0\0\0\0\0'
    libs = be(0, 0x20, 2) + b'xboxkrnl.exe\0xam.xex\0'.ljust(0x20, b'\0')
    libs = be(len(libs)) + libs[4:]
    blobs = [(0x000003FF, ff), (0x00040006, ex), (0x000183FF, pe), (0x000103FF, libs)]
    inline = [(0x00010100, 0x82000400), (0x00010201, 0x82000000), (0x00020200, 0x40000)]
    n = len(blobs) + len(inline)
    table_end = 24 + n * 8
    sec_off = table_end
    sec = bytearray(0x200)
    struct.pack_into('>I', sec, 4, 0x1200000)
    struct.pack_into('>I', sec, 0x110, 0x82000000)
    pos = sec_off + len(sec)
    table, body = b'', b''
    for k, blob in blobs:
        table += be(k, pos + len(body))
        body += blob + b'\0' * ((-len(blob)) % 4)
    for k, v in inline:
        table += be(k, v)
    hdr = magic + be(0x1, 0x10000, 0, sec_off, n)
    return hdr + table + bytes(sec) + body


class XexInfo(unittest.TestCase):
    def test_parses_headers(self):
        info = x.parse_xex(build_xex())
        self.assertEqual(info['title_id'], 0x4E4D0925)
        self.assertEqual(info['entry_point'], 0x82000400)
        self.assertEqual(info['image_base'], 0x82000000)
        self.assertEqual(info['load_address'], 0x82000000)
        self.assertEqual(info['image_size'], 0x1200000)
        self.assertEqual(info['compression'], 2)
        self.assertEqual(info['original_pe_name'], 'default.exe')
        self.assertEqual(info['import_libraries'], ['xboxkrnl.exe', 'xam.xex'])
        self.assertEqual(x.verdict(info), [])

    def test_rejects_pc_executable(self):
        with self.assertRaises(x.XexError):
            x.parse_xex(b'MZ' + b'\0' * 200)

    def test_flags_delta_patch(self):
        info = x.parse_xex(build_xex(compression=3))
        self.assertTrue(any('delta' in p for p in x.verdict(info)))

    def test_truncated_file(self):
        with self.assertRaises(x.XexError):
            x.parse_xex(build_xex()[:40])

    def test_cli(self):
        with tempfile.NamedTemporaryFile(suffix='.xex', delete=False) as f:
            f.write(build_xex())
        try:
            self.assertEqual(x.main([f.name]), 0)
        finally:
            os.unlink(f.name)


if __name__ == '__main__':
    unittest.main()
