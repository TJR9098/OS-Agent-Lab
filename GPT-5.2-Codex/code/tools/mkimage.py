#!/usr/bin/env python3
import argparse
import os
import struct

SECTOR_SIZE = 512
HEADER_SIZE = 64
IMAGE_MAGIC = 0x474d495643534952
IMAGE_VERSION = 1


def align_up(value, align):
    return (value + align - 1) // align * align


def build_image(kernel_path, initrd_path, out_path):
    with open(kernel_path, "rb") as f:
        kernel = f.read()

    initrd = b""
    if initrd_path:
        with open(initrd_path, "rb") as f:
            initrd = f.read()

    kernel_offset = SECTOR_SIZE
    kernel_size = len(kernel)
    initrd_offset = align_up(kernel_offset + kernel_size, SECTOR_SIZE)
    initrd_size = len(initrd)

    header = struct.pack(
        "<QIIQQQQQ",
        IMAGE_MAGIC,
        IMAGE_VERSION,
        HEADER_SIZE,
        kernel_offset,
        kernel_size,
        initrd_offset,
        initrd_size,
        0,
    )
    if len(header) > HEADER_SIZE:
        raise ValueError("header too large")

    header = header + b"\x00" * (HEADER_SIZE - len(header))
    if len(header) > SECTOR_SIZE:
        raise ValueError("header exceeds sector size")

    os.makedirs(os.path.dirname(out_path), exist_ok=True)

    with open(out_path, "wb") as f:
        f.write(header)
        f.write(b"\x00" * (SECTOR_SIZE - len(header)))
        f.write(kernel)

        pad = initrd_offset - (kernel_offset + kernel_size)
        if pad:
            f.write(b"\x00" * pad)

        if initrd_size:
            f.write(initrd)


def main():
    parser = argparse.ArgumentParser(description="Build disk image")
    parser.add_argument("--kernel", required=True, help="Path to kernel ELF")
    parser.add_argument("--initrd", help="Path to initrd image")
    parser.add_argument("--out", default="build/disk.img", help="Output disk image")
    args = parser.parse_args()

    build_image(args.kernel, args.initrd, args.out)


if __name__ == "__main__":
    main()