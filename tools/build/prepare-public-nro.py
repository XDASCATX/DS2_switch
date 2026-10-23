#!/usr/bin/env python3
"""Repackage the v56 NRO with public metadata; leave its executable section unchanged."""

import argparse
import hashlib
import struct
from pathlib import Path


ASSET_HEADER = struct.Struct("<4sIQQQQQQ")
NACP_SIZE = 0x4000
TITLE_SIZE = 0x200
PUBLISHER_SIZE = 0x100
LANGUAGE_COUNT = 16


def clean_icon(data):
    if not data.startswith(b"\xff\xd8"):
        raise ValueError("the icon must be a JPEG")
    output = bytearray(data[:2])
    offset = 2
    dimensions = None
    while offset < len(data):
        start = offset
        if data[offset] != 0xFF:
            raise ValueError("invalid JPEG marker")
        while offset < len(data) and data[offset] == 0xFF:
            offset += 1
        if offset == len(data):
            raise ValueError("truncated JPEG marker")
        marker = data[offset]
        offset += 1
        if marker == 0xDA:  # Keep the compressed image data after the scan header intact.
            output.extend(data[start:])
            break
        if marker in (0xD8, 0xD9) or 0xD0 <= marker <= 0xD7:
            raise ValueError("unexpected JPEG marker before image data")
        if offset + 2 > len(data):
            raise ValueError("truncated JPEG segment")
        length = int.from_bytes(data[offset : offset + 2], "big")
        end = offset + length
        if length < 2 or end > len(data):
            raise ValueError("invalid JPEG segment length")
        if marker in (0xC0, 0xC1, 0xC2):
            if length < 8:
                raise ValueError("invalid JPEG frame")
            dimensions = struct.unpack_from(">HH", data, offset + 3)
        if not (0xE0 <= marker <= 0xEF or marker == 0xFE):
            output.extend(data[start:end])
        offset = end
    else:
        raise ValueError("JPEG has no image data")
    if dimensions != (256, 256) or not output.endswith(b"\xff\xd9"):
        raise ValueError("the icon must be a complete 256x256 JPEG")
    return bytes(output)


def patch_nacp(data, name, publisher, version):
    if len(data) != NACP_SIZE:
        raise ValueError("expected a 0x4000-byte NACP in the NRO")
    fields = ((name, TITLE_SIZE), (publisher, PUBLISHER_SIZE), (version, 0x10))
    for text, limit in fields:
        if not text or len(text.encode("utf-8")) >= limit:
            raise ValueError("NACP metadata is empty or too long")
    nacp = bytearray(data)
    for language in range(LANGUAGE_COUNT):
        offset = language * (TITLE_SIZE + PUBLISHER_SIZE)
        nacp[offset : offset + TITLE_SIZE] = name.encode("utf-8").ljust(TITLE_SIZE, b"\0")
        nacp[offset + TITLE_SIZE : offset + TITLE_SIZE + PUBLISHER_SIZE] = publisher.encode("utf-8").ljust(PUBLISHER_SIZE, b"\0")
    nacp[0x3060:0x3070] = version.encode("utf-8").ljust(0x10, b"\0")
    return bytes(nacp)


def repackage(nro, icon, name, publisher, version):
    if nro[0x10:0x14] != b"NRO0" or len(nro) < 0x80:
        raise ValueError("invalid NRO header")
    executable_size = struct.unpack_from("<I", nro, 0x18)[0]
    if executable_size + ASSET_HEADER.size > len(nro):
        raise ValueError("NRO asset header is truncated")
    magic, asset_version, icon_offset, icon_size, nacp_offset, nacp_size, romfs_offset, romfs_size = (
        ASSET_HEADER.unpack_from(nro, executable_size)
    )
    if magic != b"ASET" or asset_version != 0 or romfs_offset or romfs_size:
        raise ValueError("unexpected NRO assets; refusing to drop ROMFS")
    if not icon_size or not nacp_size or executable_size + nacp_offset + nacp_size != len(nro):
        raise ValueError("unexpected NRO asset layout")
    if icon_offset < ASSET_HEADER.size or nacp_offset < icon_offset + icon_size:
        raise ValueError("overlapping NRO assets")
    nacp = patch_nacp(nro[executable_size + nacp_offset :], name, publisher, version)
    icon_offset = ASSET_HEADER.size
    nacp_offset = icon_offset + len(icon)
    header = ASSET_HEADER.pack(b"ASET", 0, icon_offset, len(icon), nacp_offset, len(nacp), 0, 0)
    return nro[:executable_size] + header + icon + nacp, executable_size


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("nro", type=Path, help="existing v56 NRO (never modified)")
    parser.add_argument("icon", type=Path, help="official 256x256 JPEG icon")
    parser.add_argument("output", type=Path, help="new public NRO path (must not exist)")
    parser.add_argument("--name", default="DARK SOULS II: Scholar of the First Sin")
    parser.add_argument("--publisher", default="xDASCATx")
    parser.add_argument("--version", default="0.56")
    args = parser.parse_args()
    if args.output.exists():
        parser.error("output already exists; refusing to overwrite")
    icon = clean_icon(args.icon.read_bytes())
    original = args.nro.read_bytes()
    public, executable_size = repackage(original, icon, args.name, args.publisher, args.version)
    if public[:executable_size] != original[:executable_size]:
        raise AssertionError("the NRO executable section changed")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(public)
    digest = lambda data: hashlib.sha256(data).hexdigest()
    print("executable sha256:", digest(original[:executable_size]))
    print("public NRO sha256:", digest(public))
    print("icon sha256:", digest(icon))


if __name__ == "__main__":
    main()
