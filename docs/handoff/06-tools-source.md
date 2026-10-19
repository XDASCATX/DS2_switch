# 06. Исходники инструментов и конфигов
Полные тексты всего, что было написано в проекте. Те же файлы лежат рядом в `tools/`, `tests/`, `config/`. Из скопированного из NFSMW-NX изменён только `tools/fetch_thirdparty.py`, патч описан в `02-host-build.md`.
Зависимости: Python 3.11+ (нужен `tomllib`), пакет `cryptography`, `llvm-objdump` с поддержкой PowerPC (подойдёт LLVM 18+). Путь к дизассемблеру можно задать переменной `OBJDUMP`.

## `tools/ds2_xexinfo.py`

Разбор заголовка XEX2. Проверено на реальном файле.

```python
#!/usr/bin/env python3
"""Inspects the header of an Xbox 360 default.xex (XEX2) before it is handed to the code generator.

Checks that the file is an Xbox 360 executable (not a PC or Xbox One build), prints the title id, entry point, base
address, encryption/compression mode and imported kernel libraries, and exits non-zero if the file is something the
recompilation chain cannot use.

Usage: python tools/ds2_xexinfo.py path/to/default.xex [--json]
Only the header is read; no game data is written anywhere.
"""
import argparse
import json
import struct
import sys

ENCRYPTION = {0: 'none', 1: 'normal'}
COMPRESSION = {0: 'none', 1: 'basic', 2: 'normal (LZX)', 3: 'delta patch'}

KEY_FILE_FORMAT = 0x000003FF
KEY_ENTRY_POINT = 0x00010100
KEY_IMAGE_BASE = 0x00010201
KEY_ORIGINAL_BASE = 0x00010001
KEY_IMPORTS = 0x000103FF
KEY_ORIGINAL_PE_NAME = 0x000183FF
KEY_STACK_SIZE = 0x00020200
KEY_EXECUTION_INFO = 0x00040006


class XexError(Exception):
    pass


def u32(data, off):
    if off < 0 or off + 4 > len(data):
        raise XexError(f'read past end of file at 0x{off:X}')
    return struct.unpack_from('>I', data, off)[0]


def u16(data, off):
    if off < 0 or off + 2 > len(data):
        raise XexError(f'read past end of file at 0x{off:X}')
    return struct.unpack_from('>H', data, off)[0]


def parse_xex(data):
    magic = data[:4]
    if magic == b'XEX1':
        raise XexError('XEX1 (early beta format) is not supported')
    if magic != b'XEX2':
        raise XexError(f'not an Xbox 360 XEX2 file (magic {magic!r}); PC (MZ) and Xbox One builds are not valid inputs')

    info = {
        'module_flags': u32(data, 4),
        'pe_data_offset': u32(data, 8),
        'security_offset': u32(data, 16),
    }
    count = u32(data, 20)
    if count > 512:
        raise XexError(f'implausible optional header count {count}')

    headers = {}
    for i in range(count):
        key = u32(data, 24 + i * 8)
        val = u32(data, 28 + i * 8)
        size_class = key & 0xFF
        if size_class <= 1:
            headers[key] = val                       # value stored inline
        elif size_class == 0xFF:
            size = u32(data, val)                    # variable size, first word is the size
            headers[key] = data[val:val + size]
        else:
            headers[key] = data[val:val + size_class * 4]

    ff = headers.get(KEY_FILE_FORMAT)
    if not isinstance(ff, (bytes, bytearray)) or len(ff) < 8:
        raise XexError('missing file format info header')
    info['encryption'] = u16(ff, 4)
    info['compression'] = u16(ff, 6)

    info['entry_point'] = headers.get(KEY_ENTRY_POINT)
    info['image_base'] = headers.get(KEY_IMAGE_BASE)
    info['original_base'] = headers.get(KEY_ORIGINAL_BASE)

    sec = info['security_offset']
    info['image_size'] = u32(data, sec + 4)
    info['load_address'] = u32(data, sec + 0x110)

    ex = headers.get(KEY_EXECUTION_INFO)
    if isinstance(ex, (bytes, bytearray)) and len(ex) >= 24:
        info['title_id'] = u32(ex, 12)
        info['media_id'] = u32(ex, 0)
        info['version'] = u32(ex, 4)
        info['executable_type'] = ex[17]
        info['disc_number'] = ex[18]
        info['disc_count'] = ex[19]

    pe_name = headers.get(KEY_ORIGINAL_PE_NAME)
    if isinstance(pe_name, (bytes, bytearray)) and len(pe_name) > 4:
        info['original_pe_name'] = bytes(pe_name[4:]).split(b'\0')[0].decode('ascii', 'replace')

    stack = headers.get(KEY_STACK_SIZE)
    if isinstance(stack, int):
        info['stack_size'] = stack

    imports = headers.get(KEY_IMPORTS)
    if isinstance(imports, (bytes, bytearray)) and len(imports) >= 12:
        table_size = u32(imports, 4)
        n_libs = u32(imports, 8)
        pos = 12
        names = bytes(imports[pos:pos + table_size])
        libs = [n.decode('ascii', 'replace') for n in names.split(b'\0') if n][:n_libs]
        info['import_libraries'] = libs
    info['optional_header_keys'] = sorted(f'0x{k:08X}' for k in headers)
    return info


def verdict(info):
    problems = []
    if info['encryption'] not in ENCRYPTION:
        problems.append(f"unknown encryption type {info['encryption']}")
    if info['compression'] == 3:
        problems.append('delta-patch XEX: apply the title update to the base XEX first')
    if info['compression'] not in COMPRESSION:
        problems.append(f"unknown compression type {info['compression']}")
    if info.get('entry_point') is None:
        problems.append('no entry point header')
    return problems


def render(info):
    lines = []
    lines.append(f"title id        : {info['title_id']:08X}" if 'title_id' in info else 'title id        : (none)')
    if 'original_pe_name' in info:
        lines.append(f"original PE     : {info['original_pe_name']}")
    lines.append(f"entry point     : 0x{info['entry_point']:08X}" if info.get('entry_point') is not None else 'entry point     : (none)')
    base = info.get('image_base') if info.get('image_base') is not None else info['load_address']
    lines.append(f'image base      : 0x{base:08X}')
    lines.append(f"image size      : 0x{info['image_size']:X}")
    lines.append(f"encryption      : {ENCRYPTION.get(info['encryption'], info['encryption'])}")
    lines.append(f"compression     : {COMPRESSION.get(info['compression'], info['compression'])}")
    if 'import_libraries' in info:
        lines.append('imports         : ' + ', '.join(info['import_libraries']))
    return '\n'.join(lines)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('xex')
    ap.add_argument('--json', action='store_true')
    args = ap.parse_args(argv)
    with open(args.xex, 'rb') as f:
        data = f.read(1 << 20)  # headers always fit in the first MiB
    try:
        info = parse_xex(data)
    except XexError as e:
        print(f'error: {e}', file=sys.stderr)
        return 2
    problems = verdict(info)
    if args.json:
        print(json.dumps({**info, 'problems': problems}, default=str, indent=2))
    else:
        print(render(info))
        for p in problems:
            print(f'problem: {p}', file=sys.stderr)
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
```

## `tests/test_xexinfo.py`

5 юнит-тестов на синтетическом XEX (все проходят).

```python
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
```

## `tools/ds2_imagedump.py`

Расшифровка и распаковка PE-образа, PDATA. Проверено 19.10.2026: секции и 54 962 записи PDATA совпадают с прошлым прогоном.

```python
#!/usr/bin/env python3
"""Decrypts and unpacks the PE image of an Xbox 360 XEX2 (basic compression) for read-only analysis.

It repeats what ReXGlue's loader does (sdk/src/system/xex_module.cpp, ReadImage / ReadImageBasicCompressed), so the
result is byte-for-byte the image that the code generator analyses:

  * session key = AES-128-ECB-decrypt(retail key, security_info.aes_key)  (the key is the one in xex_module.cpp)
  * image       = AES-128-CBC-decrypt(session key, IV = 0, one chain over all data blocks); zero blocks are skipped

The input file is only read. The output goes to the path given with -o (keep it under out/, which is git-ignored).
Self-checks: MZ header, PE signature, machine type 0x01F2, PE32 magic, exception directory inside the image.
Compressed (LZX) and delta-patch XEX files are rejected.

Usage:
  python tools/ds2_imagedump.py assets/game_root/Default.xex -o out/ds2_image.bin
  python tools/ds2_imagedump.py assets/game_root/Default.xex --info
"""
import argparse
import struct
import sys

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

RETAIL_KEY = bytes([0x20, 0xB1, 0x85, 0xA5, 0x9D, 0x28, 0xFD, 0xC3, 0x40, 0x58, 0x3F, 0xBB, 0x08, 0x96, 0xBF, 0x91])
DEVKIT_KEY = bytes(16)
KEY_FILE_FORMAT = 0x000003FF


class ImageError(Exception):
    pass


def be32(d, o):
    return struct.unpack_from('>I', d, o)[0]


def be16(d, o):
    return struct.unpack_from('>H', d, o)[0]


def _aes_cbc_decrypt(key, data):
    if len(data) % 16:
        raise ImageError('encrypted data is not a multiple of 16 bytes')
    dec = Cipher(algorithms.AES(key), modes.CBC(bytes(16))).decryptor()
    return dec.update(data) + dec.finalize()


def _optional_header(d, wanted):
    count = be32(d, 20)
    for i in range(count):
        key, val = be32(d, 24 + i * 8), be32(d, 28 + i * 8)
        if key != wanted:
            continue
        sc = key & 0xFF
        if sc <= 1:
            return val
        size = be32(d, val) if sc == 0xFF else sc * 4
        return d[val:val + size]
    return None


def extract_image(xex, use_dev_key=False):
    """Returns (image bytes, load_address, image_size). Raises ImageError on anything unexpected."""
    if xex[:4] != b'XEX2':
        raise ImageError('not a XEX2 file')
    pe_offset = be32(xex, 8)
    sec = be32(xex, 16)
    ff = _optional_header(xex, KEY_FILE_FORMAT)
    if not isinstance(ff, (bytes, bytearray)):
        raise ImageError('missing file format info')
    info_size, enc, comp = be32(ff, 0), be16(ff, 4), be16(ff, 6)
    if enc != 1:
        raise ImageError(f'unsupported encryption type {enc} (only "normal" is handled here)')
    if comp != 1:
        raise ImageError(f'unsupported compression type {comp} (only "basic" is handled here)')
    image_size = be32(xex, sec + 4)
    load_address = be32(xex, sec + 0x110)
    session = Cipher(algorithms.AES(DEVKIT_KEY if use_dev_key else RETAIL_KEY), modes.ECB()).decryptor().update(
        bytes(xex[sec + 0x150:sec + 0x160]))
    blocks = [struct.unpack_from('>II', ff, 8 + i * 8) for i in range((info_size - 8) // 8)]
    total = sum(a + b for a, b in blocks)
    if total != image_size:
        raise ImageError(f'basic blocks cover 0x{total:X} bytes but the security info says 0x{image_size:X}')
    data_total = sum(a for a, _ in blocks)
    if pe_offset + data_total != len(xex):
        raise ImageError('file size does not match header size + data blocks (truncated or padded file?)')
    plain = _aes_cbc_decrypt(session, bytes(xex[pe_offset:pe_offset + data_total]))
    image = bytearray(image_size)
    src = dst = 0
    for data_size, zero_size in blocks:
        image[dst:dst + data_size] = plain[src:src + data_size]
        src += data_size
        dst += data_size + zero_size
    return bytes(image), load_address, image_size


def parse_pe(image, base):
    """Minimal PE parser. PE headers are little-endian (only the code and PDATA are big-endian)."""
    le32 = lambda o: struct.unpack_from('<I', image, o)[0]
    le16 = lambda o: struct.unpack_from('<H', image, o)[0]
    if image[:2] != b'MZ':
        raise ImageError('decrypted image does not start with MZ (wrong key or damaged file)')
    lfanew = le32(0x3C)
    if lfanew + 0x100 > len(image) or image[lfanew:lfanew + 4] != b'PE\0\0':
        raise ImageError('missing PE signature')
    machine = le16(lfanew + 4)
    if machine != 0x01F2:
        raise ImageError(f'unexpected machine type 0x{machine:04X} (expected 0x01F2, PowerPC big-endian)')
    n_sections = le16(lfanew + 6)
    opt_size = le16(lfanew + 20)
    opt = lfanew + 24
    magic = le16(opt)
    if magic != 0x10B or n_sections > 96:
        raise ImageError('implausible PE optional header')
    image_base = le32(opt + 28)
    exc_rva, exc_size = le32(opt + 96 + 3 * 8), le32(opt + 96 + 3 * 8 + 4)
    sections = []
    sh = opt + opt_size
    for i in range(n_sections):
        o = sh + i * 40
        name = image[o:o + 8].rstrip(b'\0').decode('ascii', 'replace')
        vsize, vaddr, rawsize, rawptr = struct.unpack_from('<IIII', image, o + 8)
        chars = le32(o + 36)
        # Sections such as .reloc may be described in the header but not mapped into the loaded image.
        sections.append({'name': name, 'va': vaddr, 'vsize': vsize, 'chars': chars,
                         'mapped': vaddr + vsize <= len(image)})
    return {'machine': machine, 'magic': magic, 'image_base': image_base, 'sections': sections,
            'exc_rva': exc_rva, 'exc_size': exc_size, 'n_sections': n_sections}


def pdata_functions(image, base, pe):
    """Returns sorted list of (begin_va, size_bytes, prolog_len, exception_flag) from the exception directory."""
    start = pe['exc_rva']
    if start >= base:  # directory address may be stored as an absolute VA
        start -= base
    if start + pe['exc_size'] > len(image):
        raise ImageError('exception directory lies outside the image')
    out = []
    for o in range(start, start + pe['exc_size'], 8):
        begin, data = be32(image, o), be32(image, o + 4)
        if begin == 0:
            continue
        prolog = data & 0xFF
        length = (data >> 8) & 0x3FFFFF
        exc = (data >> 31) & 1
        out.append((begin, max(length * 4, 4), prolog, exc))
    out.sort()
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('xex')
    ap.add_argument('-o', '--out', help='write the raw image (base address = load address) to this file')
    ap.add_argument('--info', action='store_true', help='print PE sections and PDATA statistics')
    ap.add_argument('--dev-key', action='store_true')
    args = ap.parse_args(argv)
    with open(args.xex, 'rb') as f:
        xex = f.read()
    try:
        image, base, size = extract_image(xex, args.dev_key)
        pe = parse_pe(image, base)
    except ImageError as e:
        print(f'error: {e}', file=sys.stderr)
        return 2
    print(f'image: {size} bytes (0x{size:X}) at 0x{base:08X}; MZ ok; machine 0x{pe["machine"]:04X}; '
          f'PE32 magic 0x{pe["magic"]:X}; PE image base 0x{pe["image_base"]:08X}')
    if args.info or not args.out:
        for s in pe['sections']:
            note = '' if s['mapped'] else '  (not mapped in the loaded image)'
            print(f'  {s["name"]:<8} va 0x{base + s["va"]:08X} size 0x{s["vsize"]:08X} flags 0x{s["chars"]:08X}{note}')
        pd = pdata_functions(image, base, pe)
        print(f'  PDATA: {len(pd)} entries, exception directory rva 0x{pe["exc_rva"]:X} size 0x{pe["exc_size"]:X}')
    if args.out:
        with open(args.out, 'wb') as f:
            f.write(image)
        print(f'wrote {args.out}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
```

## `tools/ds2_make_elf.py`

Обёртка образа в ELF по адресу 0x82000000 для llvm-objdump. Новая: заменяет `llvm-objcopy --change-section-address`, которого нет в LLVM 18.

```python
#!/usr/bin/env python3
"""Wraps the raw image from ds2_imagedump.py in a minimal ELF32 big-endian PowerPC file with one .text section at
0x82000000, so `llvm-objdump -d --triple=powerpc64-unknown-unknown` prints real guest addresses.
Needs no objcopy (older llvm-objcopy lacks --change-section-address).

Usage: python tools/ds2_make_elf.py out/ds2_image.bin out/ds2_image.elf
"""
import struct
import sys

BASE = 0x82000000


def main(src, dst):
    data = open(src, 'rb').read()
    shstr = b'\0.text\0.shstrtab\0'
    eh = 52
    off_data = eh
    off_shstr = off_data + len(data)
    off_sh = (off_shstr + len(shstr) + 3) & ~3
    e = bytearray(b'\x7fELF\x01\x02\x01' + bytes(9))  # ELFCLASS32, big-endian, version 1
    e += struct.pack('>HHIIIIIHHHHHH', 2, 20, 1, BASE, 0, off_sh, 0, eh, 0, 0, 40, 3, 2)  # EM_PPC = 20
    sh = lambda n, t, f, a, o, s, al: struct.pack('>IIIIIIIIII', n, t, f, a, o, s, 0, 0, al, 0)
    out = bytes(e) + data + shstr
    out += bytes(off_sh - len(out))
    out += sh(0, 0, 0, 0, 0, 0, 0) + sh(1, 1, 6, BASE, off_data, len(data), 4) + sh(7, 3, 0, 0, off_shstr, len(shstr), 1)
    open(dst, 'wb').write(out)
    print('wrote', dst)


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
```

## `tools/ds2_analyze_unresolved.py`

Контекст и перекрёстные ссылки по 50 целям. Проверено: self-check 50/50, те же счётчики, что в прошлой сессии.

```python
#!/usr/bin/env python3
"""Explains the `UnresolvedCall` errors of a failed `rexglue codegen` run, using only the decrypted XEX image.

Inputs : out/ds2_image.bin (from ds2_imagedump.py), the codegen log, out/ds2_image.elf (for llvm-objdump text).
Output : a per-target report (out/reports/unresolved_targets.txt) and a summary on stdout. Nothing is modified.

For every logged `b 0xTARGET from 0xSOURCE` it collects instruction-level evidence:
  * the source instruction really is `b TARGET` (checks that the dump matches what codegen saw)
  * whether source/target lie inside a PDATA (exception table) function
  * the instruction in front of the target (terminator = blr/bctr/b/padding, otherwise code falls into it)
  * how the target starts (frame-creating prologue, plain leaf code, epilogue-only code)
  * who refers to the target in the whole image: direct bl/b/bc branches, 32-bit pointers in .rdata/.data
    (vtables, function pointers) and lis+addi/ori address constructions
and derives a rough automatic classification (the manual review in 05-unresolved-analysis.md supersedes it).

Disassembler: set OBJDUMP env var (default: llvm-objdump-20, falls back to llvm-objdump).
"""
import argparse
import bisect
import collections
import os
import re
import shutil
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
import ds2_imagedump as D  # noqa: E402

BASE = 0x82000000
BLR, BCTR, NOP = 0x4E800020, 0x4E800420, 0x60000000
OBJDUMP = os.environ.get('OBJDUMP') or shutil.which('llvm-objdump-20') or shutil.which('llvm-objdump') or 'llvm-objdump'


def sext(v, bits):
    return v - (1 << bits) if v & (1 << (bits - 1)) else v


def mnem(w):
    """Very small decoder, enough for structural classification."""
    op = w >> 26
    if w == BLR:
        return 'blr'
    if w == BCTR:
        return 'bctr'
    if w == 0x4E800421:
        return 'bctrl'
    if w == NOP:
        return 'nop'
    if w == 0:
        return 'zero'
    if op == 18:
        return 'bl' if w & 1 else 'b'
    if op == 16:
        return 'bc'
    if (w & 0xFC1FFFFF) == 0x7C0802A6:
        return 'mflr'
    if (w & 0xFC1FFFFF) == 0x7C0803A6:
        return 'mtlr'
    ra, rs = (w >> 16) & 31, (w >> 21) & 31
    if op == 37 and ra == 1:
        return 'stwu_r1'
    if op == 62 and (w & 3) == 1 and ra == 1:
        return 'stdu_r1'
    if op in (36, 62) and ra == 1:
        return 'st_r1'
    if op in (32, 58) and ra == 1:
        return 'ld_r1'
    if op == 14 and rs == 1 and ra == 1:
        return 'addi_r1'
    return 'other'


def load(args):
    img = open(args.image, 'rb').read()
    pe = D.parse_pe(img, BASE)
    pd = D.pdata_functions(img, BASE, pe)
    secs = {s['name']: (BASE + s['va'], BASE + s['va'] + s['vsize']) for s in pe['sections'] if s['mapped']}
    return img, pe, pd, secs


def parse_log(path):
    txt = open(path, encoding='utf-8', errors='replace').read()
    return [(int(t, 16), int(s, 16)) for t, s in re.findall(r'^\s+0x([0-9A-F]{8}) from 0x([0-9A-F]{8}): b ', txt, re.M)]


def disasm(elf, start, stop):
    r = subprocess.run([OBJDUMP, '-d', '--triple=powerpc64-unknown-unknown', '--mattr=+altivec',
                        f'--start-address=0x{start:08X}', f'--stop-address=0x{stop:08X}', elf],
                       capture_output=True, text=True)
    lines = []
    for ln in r.stdout.splitlines():
        m = re.match(r'\s*([0-9a-f]{8}):\s+(?:[0-9a-f]{2} ){4}\s*(.*)', ln)
        if m:
            txt = re.sub(r'\s*<(?:_binary|\.text)[^>]*>', '', m.group(2))
            lines.append(f'{m.group(1)}: {txt}')
    return lines


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--image', default='out/ds2_image.bin')
    ap.add_argument('--elf', default='out/ds2_image.elf')
    ap.add_argument('--log', default='out/logs/runner.out')
    ap.add_argument('--report', default='out/reports/unresolved_targets.txt')
    args = ap.parse_args()

    img, pe, pd, secs = load(args)
    rows = parse_log(args.log)
    word = lambda a: struct.unpack_from('>I', img, a - BASE)[0]
    starts = [b for b, _, _, _ in pd]

    def pdfind(a):
        i = bisect.bisect_right(starts, a) - 1
        return pd[i] if i >= 0 and pd[i][0] <= a < pd[i][0] + pd[i][1] else None

    wanted = {t for t, _ in rows}
    # 1. self-check against what codegen saw
    ok = 0
    for t, s in rows:
        w = word(s)
        if w >> 26 == 18 and not w & 3 and (s + sext(w & 0x03FFFFFC, 26)) & 0xFFFFFFFF == t:
            ok += 1
    print(f'self-check: {ok}/{len(rows)} logged sources are `b <target>` in the dump')

    # 2. cross references over the whole image
    tlo, thi = secs['.text']
    text = img[tlo - BASE:thi - BASE]
    n = len(text) // 4
    words = struct.unpack('>%dI' % n, text[:n * 4])
    bl_in, b_in, bc_in, addr_in = (collections.defaultdict(list) for _ in range(4))
    for i, w in enumerate(words):
        a = tlo + i * 4
        op = w >> 26
        if op == 18:
            li = sext(w & 0x03FFFFFC, 26)
            t = li & 0xFFFFFFFF if w & 2 else (a + li) & 0xFFFFFFFF
            if t in wanted:
                (bl_in if w & 1 else b_in)[t].append(a)
        elif op == 16:
            bd = sext(w & 0xFFFC, 16)
            t = bd & 0xFFFFFFFF if w & 2 else (a + bd) & 0xFFFFFFFF
            if t in wanted:
                bc_in[t].append(a)
        elif op == 15 and ((w >> 16) & 31) == 0:  # lis rD, hi
            rd, hi = (w >> 21) & 31, w & 0xFFFF
            for k in range(1, 5):
                if i + k >= n:
                    break
                w2 = words[i + k]
                if (w2 >> 21) & 31 == rd and (w2 >> 16) & 31 == rd:
                    if w2 >> 26 == 14:
                        v = ((hi << 16) + sext(w2 & 0xFFFF, 16)) & 0xFFFFFFFF
                    elif w2 >> 26 == 24:
                        v = (hi << 16) | (w2 & 0xFFFF)
                    else:
                        continue
                    if v in wanted:
                        addr_in[v].append(a)
                    break
    data_in = collections.defaultdict(list)
    for name in ('.rdata', '.data'):
        lo, hi = secs[name]
        seg = img[lo - BASE:hi - BASE]
        for i in range(0, len(seg) - 3, 4):
            v = struct.unpack_from('>I', seg, i)[0]
            if v in wanted:
                data_in[v].append((name, lo + i))

    # 3. per-target evidence and rough classification
    os.makedirs(os.path.dirname(args.report), exist_ok=True)
    out = open(args.report, 'w')
    table = []
    src_of = dict(rows)
    for t in sorted(src_of):
        src = src_of[t]
        prev = word(t - 4)
        pk = mnem(prev)
        first = [mnem(word(t + 4 * k)) for k in range(6)]
        prologue = 'mflr' in first[:4] and any(x in ('stwu_r1', 'stdu_r1') for x in first[:6])
        epilogue_only = first[0] in ('addi_r1', 'ld_r1', 'mtlr')
        terminator_before = pk in ('blr', 'bctr', 'b', 'zero')
        n_bl, n_data, n_addr = len(bl_in[t]), len(data_in[t]), len(addr_in[t])
        n_b, n_bc = len(set(b_in[t])), len(bc_in[t])
        if n_bl + n_data + n_addr:
            cls = 'standalone (entry evidence)'
        elif terminator_before and prologue:
            cls = 'standalone (prologue after terminator)'
        elif epilogue_only and not terminator_before:
            cls = 'continuation (epilogue-only, falls in from above)'
        elif not terminator_before:
            cls = 'continuation? (code falls into target)'
        elif epilogue_only:
            cls = 'shared epilogue chunk'
        else:
            cls = 'unclear (needs context)'
        table.append(dict(t=t, s=src, cls=cls, n_bl=n_bl, n_data=n_data, n_addr=n_addr,
                          pd_t=pdfind(t), pd_s=pdfind(src)))
        out.write(f'=== target 0x{t:08X}  from 0x{src:08X}  -> {cls}\n')
        out.write(f'    refs to target: bl={n_bl} b={n_b} bc={n_bc} data={n_data} lis+addi={n_addr}; '
                  f'prev={pk}; first={first}\n')
        if data_in[t]:
            out.write('    data refs: ' + ', '.join(f'{nm}@0x{a:08X}' for nm, a in data_in[t][:6]) + '\n')
        if bl_in[t]:
            out.write('    bl callers: ' + ', '.join(f'0x{a:08X}' for a in bl_in[t][:6]) + '\n')
        out.write('  -- source context\n')
        for ln in disasm(args.elf, src - 16, src + 8):
            out.write('    ' + ln + '\n')
        out.write('  -- target context\n')
        for ln in disasm(args.elf, t - 12, t + 40):
            out.write('    ' + ln + '\n')
        out.write('\n')
    out.close()

    c = collections.Counter(r['cls'] for r in table)
    print('classification counts:')
    for k, v in c.most_common():
        print(f'  {v:3d}  {k}')
    print('sources inside PDATA function:', sum(1 for r in table if r['pd_s']), '/', len(table))
    print('targets inside PDATA function:', sum(1 for r in table if r['pd_t']), '/', len(table))
    print('targets with >=1 bl caller   :', sum(1 for r in table if r['n_bl']))
    print('targets referenced from data :', sum(1 for r in table if r['n_data']))
    print('targets built via lis+addi   :', sum(1 for r in table if r['n_addr']))
    print('report ->', args.report)


if __name__ == '__main__':
    main()
```

## `tools/ds2_classify_unresolved.py`

Автоклассификация и проверка хвостовых целей. **Ни разу не запускался.**

```python
#!/usr/bin/env python3
"""Classifies the `UnresolvedCall` targets of a failed `rexglue codegen` run from instruction evidence (read-only).

STATUS: written in the previous session but NEVER RUN. Treat its output as unverified until checked against
05-unresolved-analysis.md. Its main purpose is the second-order check: after declaring a thunk/stub as a function,
would its own tail-call target still be unknown (=> a new UnresolvedCall on the next codegen run)?

Needs out/ds2_image.bin (tools/ds2_imagedump.py) and the codegen log. Writes out/reports/classification.tsv.

Classes:
  THUNK         `lwz r3,N(r3)` / `addi r3,r3,N` then `b X`; a self-contained function
  LEAF_TAILCALL block preceded by a terminator, ending in a terminator (includes vtable-dispatch stubs)
  JOIN_POINT    target lies inside the control-flow reach of an entry that has only data-pointer evidence
  JOIN_POINT_UNREFERENCED  same, but the owning entry has no reference of any kind in the image
"""
import argparse
import bisect
import collections
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(__file__))
import ds2_analyze_unresolved as A  # noqa: E402
import ds2_imagedump as D  # noqa: E402

B = 0x82000000
sx = A.sext


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--image', default='out/ds2_image.bin')
    ap.add_argument('--log', default='out/logs/runner.out')
    ap.add_argument('--out', default='out/reports/classification.tsv')
    args = ap.parse_args()

    img = open(args.image, 'rb').read()
    pe = D.parse_pe(img, B)
    pd = D.pdata_functions(img, B, pe)
    secs = {s['name']: (B + s['va'], B + s['va'] + s['vsize']) for s in pe['sections'] if s['mapped']}
    rows = A.parse_log(args.log)
    w = lambda a: struct.unpack_from('>I', img, a - B)[0]
    starts = [b for b, _, _, _ in pd]
    ends = [b + s for b, s, _, _ in pd]

    def pd_inside(a):
        i = bisect.bisect_right(starts, a) - 1
        return i >= 0 and starts[i] <= a < ends[i]

    def gap_of(a):
        i = bisect.bisect_right(starts, a) - 1
        g0 = ends[i] if i >= 0 else secs['.text'][0]
        g1 = starts[i + 1] if i + 1 < len(starts) else secs['.text'][1]
        return g0, g1

    gaps = sorted({gap_of(x) for t, s in rows for x in (t, s)})
    inside = lambda a: any(g0 <= a < g1 for g0, g1 in gaps)

    tlo, thi = secs['.text']
    n = (thi - tlo) // 4
    words = struct.unpack('>%dI' % n, img[tlo - B:tlo - B + n * 4])
    all_bl = set()
    refs = collections.defaultdict(lambda: collections.defaultdict(list))
    for i, x in enumerate(words):
        a = tlo + i * 4
        op = x >> 26
        if op == 18:
            li = sx(x & 0x03FFFFFC, 26)
            t = li & 0xFFFFFFFF if x & 2 else (a + li) & 0xFFFFFFFF
            if x & 1:
                all_bl.add(t)
            if inside(t):
                refs[t]['bl' if x & 1 else 'b'].append(a)
        elif op == 16:
            bd = sx(x & 0xFFFC, 16)
            t = bd & 0xFFFFFFFF if x & 2 else (a + bd) & 0xFFFFFFFF
            if inside(t):
                refs[t]['bc'].append(a)
        elif op == 15 and ((x >> 16) & 31) == 0:
            rd, hi = (x >> 21) & 31, x & 0xFFFF
            for k in range(1, 5):
                if i + k >= n:
                    break
                w2 = words[i + k]
                if (w2 >> 21) & 31 == rd and (w2 >> 16) & 31 == rd and w2 >> 26 in (14, 24):
                    v = ((hi << 16) + sx(w2 & 0xFFFF, 16)) & 0xFFFFFFFF if w2 >> 26 == 14 else (hi << 16) | (w2 & 0xFFFF)
                    if inside(v):
                        refs[v]['lis'].append(a)
                    break
    for name in ('.rdata', '.data'):
        lo, hi = secs[name]
        seg = img[lo - B:hi - B]
        for off in range(0, len(seg) - 3, 4):
            v = struct.unpack_from('>I', seg, off)[0]
            if v & 3 == 0 and inside(v):
                refs[v]['data'].append(lo + off)
    entry_kinds = lambda a: {k for k in ('bl', 'data', 'lis') if refs.get(a, {}).get(k)}

    def reach(entry, g):
        g0, g1 = g
        seen, wl = set(), [entry]
        while wl:
            a = wl.pop()
            while g0 <= a < g1 and a not in seen:
                seen.add(a)
                x = w(a)
                op = x >> 26
                if op == 18:
                    li = sx(x & 0x03FFFFFC, 26)
                    t = li & 0xFFFFFFFF if x & 2 else (a + li) & 0xFFFFFFFF
                    if x & 1:
                        a += 4
                        continue
                    if g0 <= t < g1:
                        wl.append(t)
                    break
                if op == 16:
                    bd = sx(x & 0xFFFC, 16)
                    t = bd & 0xFFFFFFFF if x & 2 else (a + bd) & 0xFFFFFFFF
                    if g0 <= t < g1:
                        wl.append(t)
                    a += 4
                    continue
                if op == 19 and ((x >> 1) & 0x3FF) in (16, 528) and (((x >> 21) & 31) & 0x14) == 0x14 and not x & 1:
                    break
                a += 4
        return seen

    def is_thunk(t):
        i1, i2 = w(t), w(t + 4)
        adj = (i1 >> 26 in (32, 14)) and ((i1 >> 21) & 31) == 3 and ((i1 >> 16) & 31) == 3
        return adj and i2 >> 26 == 18 and not i2 & 1 and not i2 & 2

    def block_exits(t, limit=24):
        a, exits = t, []
        for k in range(limit):
            x = w(a)
            op = x >> 26
            if op == 18 and not x & 1:
                li = sx(x & 0x03FFFFFC, 26)
                exits.append((li & 0xFFFFFFFF) if x & 2 else (a + li) & 0xFFFFFFFF)
                return 'b', exits, k + 1
            if x == 0x4E800020:
                return 'blr', exits, k + 1
            if x == 0x4E800420:
                return 'bctr', exits, k + 1
            if op == 16:
                bd = sx(x & 0xFFFC, 16)
                tt = bd & 0xFFFFFFFF if x & 2 else (a + bd) & 0xFFFFFFFF
                if abs(tt - t) > 0x80:
                    exits.append(tt)
            a += 4
        return 'long', exits, limit

    def prev_kind(t):
        p = w(t - 4)
        if p == 0x4E800020:
            return 'blr'
        if p == 0x4E800420:
            return 'bctr'
        if p >> 26 == 18 and not p & 1:
            return 'b'
        if p == 0:
            return 'pad'
        return 'falls'

    out_rows = []
    for t, s in sorted(rows):
        g = gap_of(t)
        owners = [e for e in sorted(a for a in range(g[0], g[1], 4) if entry_kinds(a)) if e != t and t in reach(e, g)]
        pk = prev_kind(t)
        if owners:
            o = owners[-1]
            cls, fix = 'JOIN_POINT', o
            ev = f'reached from entry 0x{o:08X} ({"/".join(sorted(entry_kinds(o)))} refs only); prev={pk}'
        elif pk == 'falls':
            cand = t
            while True:
                cand -= 4
                p = w(cand - 4)
                if cand < g[0] or p in (0x4E800020, 0x4E800420, 0) or (p >> 26 == 18 and not p & 1):
                    break
            cls, fix = 'JOIN_POINT_UNREFERENCED', cand
            ev = f'falls into target; nearest start after a terminator 0x{cand:08X}, no reference found'
        elif is_thunk(t):
            cls, fix = 'THUNK', t
            ev = f'{A.mnem(w(t))}+b 0x{block_exits(t)[1][0]:08X}; prev={pk}'
        else:
            kind, ex, ln = block_exits(t)
            cls, fix = 'LEAF_TAILCALL', t
            ev = f'self-contained block of {ln} instr ending in {kind}; prev={pk}'
        second = []
        if cls in ('THUNK', 'LEAF_TAILCALL'):
            for x in block_exits(t)[1]:
                if x in starts or x in all_bl:
                    second.append(f'0x{x:08X}:known')
                elif pd_inside(x):
                    second.append(f'0x{x:08X}:inside-pdata')
                else:
                    second.append(f'0x{x:08X}:UNKNOWN')
        out_rows.append((t, s, cls, fix, ev, ';'.join(second)))

    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, 'w') as f:
        f.write('target\tsource\tclass\tdeclare_entry\tevidence\texits_after_declaring\n')
        for t, s, cls, fix, ev, sec in out_rows:
            f.write(f'0x{t:08X}\t0x{s:08X}\t{cls}\t0x{fix:08X}\t{ev}\t{sec}\n')
    print('classes:', dict(collections.Counter(r[2] for r in out_rows)))
    print('distinct entries to declare:', len({r[3] for r in out_rows}))
    unknown = [(r[0], r[5]) for r in out_rows if 'UNKNOWN' in r[5]]
    print('targets whose own tail-call exit would still be unknown:', len(unknown))
    for t, sec in unknown:
        print(f'   0x{t:08X}: {sec}')
    print('wrote', args.out)


if __name__ == '__main__':
    main()
```

## `tools/run_codegen_monitored.sh`

Запуск codegen с замером времени и памяти.

```bash
#!/usr/bin/env bash
# Runs rexglue codegen on the DS2 manifest and records wall time and peak memory. Not run automatically.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p out/logs
/usr/bin/time -v -o out/logs/codegen_time.txt \
  sdk/out/linux-amd64/rexglue --log-file out/logs/codegen.log codegen app/ds2_manifest.toml \
  2>&1 | tee out/logs/runner.out
grep -E "Maximum resident|Elapsed" out/logs/codegen_time.txt
```

## `config/ds2_manifest.toml`

Манифест первого прогона (в репозитории — `app/ds2_manifest.toml`).

```toml
# Dark Souls II: Scholar of the First Sin (Xbox 360), title 465307E4.
# Only fields confirmed from the XEX header / required by the SDK. No overrides yet: the first codegen run decides
# what (switch tables, gaps, chunks) has to be declared.

[project]
name = "ds2"
sdk_version = "0.10.0"
game_root = "../assets/game_root"

[entrypoint]
file_path = "../assets/game_root/Default.xex"   # exact case: the host filesystem is case-sensitive
out_directory_path = "generated/default"
```

## `config/ds2_overrides.proposed.toml`

Черновик исправления 50 ошибок. **Не применён.** Покрытие всех 50 целей проверено скриптом.

```toml
# PROPOSED, NOT APPLIED. Fix for the 50 UnresolvedCall errors of codegen run 1 (see 05-unresolved-analysis.md).
# Apply only with the project owner's approval; check how nfsmw_manifest.toml includes huecos.toml first.

[functions]
# --- A. this-adjusting thunks: `lwz r3,N(r3)` / `addi r3,r3,N` + `b X`, preceded by a terminator ---
"0x823A4748" = {}
"0x823A4750" = {}
"0x823A4758" = {}
"0x823A4760" = {}
"0x823A4790" = {}
"0x823A4798" = {}
"0x823A47A0" = {}
"0x823A47A8" = {}
"0x823A4850" = {}
"0x823A4BB8" = {}
"0x823A4BC0" = {}
"0x823A4BC8" = {}
"0x823A4BD0" = {}
"0x823A4BD8" = {}
"0x823A4BE0" = {}
"0x823A4BF0" = {}
"0x823A4D38" = {}
"0x823A4D90" = {}
"0x823A9978" = {}
"0x823DEDD8" = {}
"0x823DEDE0" = {}
"0x82406940" = {}
"0x8256A7D0" = {}
"0x82717150" = {}
"0x826EBF18" = {}
# --- B. virtual-call stubs: load vtable slot from r3, mtctr, bctr ---
"0x828C7B10" = {}
"0x828C7B88" = {}
"0x828C7C38" = {}
"0x828C7C48" = {}
"0x828C7C58" = {}
"0x828C7C68" = {}
"0x82FBBA00" = {}
# --- C. small self-contained functions without PDATA ---
"0x822844D0" = {}
"0x824813E0" = {}
"0x82481408" = {}
"0x826EBEE8" = {}
"0x826EBF60" = {}
"0x826EC020" = {}
"0x828D3080" = {}
"0x82F66FF0" = {}
"0x8332E2C8" = {}
# --- D. owners of join points (only data/vtable references to the owner) ---
"0x82646FB0" = { end = 0x82647088 }   # contains target 0x82646FF0, source 0x82647078
"0x82647360" = { end = 0x82647438 }   # contains target 0x826473A0, source 0x82647428
"0x82647718" = { end = 0x826477F0 }   # contains target 0x82647758, source 0x826477E0
"0x82647E20" = { end = 0x82647EF8 }   # contains target 0x82647E60, source 0x82647EE8
"0x82648200" = { end = 0x826482D8 }   # contains target 0x82648240, source 0x826482C8
"0x8284B7E8" = { end = 0x8284B878 }   # contains target 0x8284B85C, source 0x8284B86C
"0x828D8888" = { end = 0x828D8910 }   # contains target 0x828D88F4, source 0x828D8904
"0x828D89A0" = { end = 0x828D8A28 }   # contains target 0x828D8A0C, source 0x828D8A1C
# --- E. owner of a join point, no reference to it found ---
"0x82CFF028" = { end = 0x82CFF098 }   # contains target 0x82CFF064, source 0x82CFF094
```

## `config/gitignore.txt`

`.gitignore` рабочей папки.

```text
out/
assets/
*.log
app/generated/
sdk/thirdparty/*/.git
sdk/out/
```

## `config/unresolved_pairs.txt`

50 пар «цель источник» из лога первого прогона.

```text
822844D0 82677E44
823A4748 823A98A4
823A4750 823A98AC
823A4758 823A98B4
823A4760 823A98BC
823A4790 823A98DC
823A4798 823A98E4
823A47A0 823A98EC
823A47A8 823A98F4
823A4850 823A9994
823A4BB8 823A9A44
823A4BC0 823A9A4C
823A4BC8 823A9A54
823A4BD0 823A9A5C
823A4BD8 823A9A64
823A4BE0 823A9A74
823A4BF0 823A9A84
823A4D38 823A9BA4
823A4D90 823A9C5C
823A9978 82373D34
823DEDD8 823E17E0
823DEDE0 823E1848
82406940 823E4F60
824813E0 8268C6C8
82481408 8268CFC4
8256A7D0 8256C458
82646FF0 82647078
826473A0 82647428
82647758 826477E0
82647E60 82647EE8
82648240 826482C8
826EBEE8 8271F178
826EBF18 8271F188
826EBF60 8271F220
826EC020 8271F2C4
82717150 826EBFFC
8284B85C 8284B86C
828C7B10 828E9974
828C7B88 828D4D04
828C7C38 828553C8
828C7C48 828553C0
828C7C58 828553B8
828C7C68 828553B0
828D3080 828D4B4C
828D88F4 828D8904
828D8A0C 828D8A1C
82CFF064 82CFF094
82F66FF0 826366F4
82FBBA00 830A6B88
8332E2C8 8332B9D4
```
