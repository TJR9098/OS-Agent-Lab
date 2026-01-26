#!/usr/bin/env python3
import argparse
import os
import struct

SECTOR_SIZE = 512
PARTITION_TYPE_FAT32 = 0x0C
MBR_SIGNATURE = 0xAA55
FAT32_EOC = 0x0FFFFFFF

FAT_ATTR_DIR = 0x10
FAT_ATTR_ARCHIVE = 0x20
FAT_ATTR_LFN = 0x0F


def parse_size(value: str) -> int:
    v = value.strip().lower()
    if v.endswith("m"):
        return int(v[:-1]) * 1024 * 1024
    if v.endswith("g"):
        return int(v[:-1]) * 1024 * 1024 * 1024
    return int(v)


def write_at(f, offset: int, data: bytes) -> None:
    f.seek(offset)
    f.write(data)


def compute_fat_size(total_sectors: int, reserved_sectors: int, num_fats: int, sectors_per_cluster: int) -> int:
    fat_sectors = 1
    while True:
        data_sectors = total_sectors - reserved_sectors - num_fats * fat_sectors
        cluster_count = data_sectors // sectors_per_cluster
        new_fat = (cluster_count * 4 + SECTOR_SIZE - 1) // SECTOR_SIZE
        if new_fat == fat_sectors:
            return fat_sectors
        fat_sectors = new_fat


def is_invalid_char(c: str) -> bool:
    if not c:
        return True
    if ord(c) < 0x20 or ord(c) == 0x7F:
        return True
    if ord(c) >= 0x80:
        return True
    if c in ['"', '*', '/', '\\', ':', '<', '>', '?', '|']:
        return True
    return False


def is_short_char(c: str) -> bool:
    if 'A' <= c <= 'Z':
        return True
    if '0' <= c <= '9':
        return True
    if c in ['$', '%', "'", '-', '_', '@', '~', '!', '#', '&', '(', ')', '^']:
        return True
    return False


def ensure_ascii_name(name: str) -> None:
    if not name:
        raise ValueError("empty name")
    for ch in name:
        if is_invalid_char(ch):
            raise ValueError(f"invalid name: {name}")


def lfn_checksum(shortname: bytes) -> int:
    total = 0
    for b in shortname:
        total = ((total & 1) << 7) + (total >> 1) + b
        total &= 0xFF
    return total


def lfn_char(name: str, idx: int) -> int:
    if idx < len(name):
        return ord(name[idx])
    if idx == len(name):
        return 0x0000
    return 0xFFFF


def build_lfn_entry(name: str, ord_idx: int, checksum: int) -> bytes:
    entry = bytearray(32)
    entry[0] = ord_idx
    entry[11] = FAT_ATTR_LFN
    entry[12] = 0
    entry[13] = checksum
    entry[26:28] = b"\x00\x00"

    base = (ord_idx & 0x1F) - 1
    base *= 13

    name1 = [lfn_char(name, base + i) for i in range(5)]
    name2 = [lfn_char(name, base + 5 + i) for i in range(6)]
    name3 = [lfn_char(name, base + 11 + i) for i in range(2)]

    off = 1
    for c in name1:
        entry[off:off + 2] = struct.pack('<H', c)
        off += 2
    off = 14
    for c in name2:
        entry[off:off + 2] = struct.pack('<H', c)
        off += 2
    off = 28
    for c in name3:
        entry[off:off + 2] = struct.pack('<H', c)
        off += 2

    return bytes(entry)


def build_short_entry(shortname: bytes, attr: int, cluster: int, size: int) -> bytes:
    entry = bytearray(32)
    entry[0:11] = shortname
    entry[11] = attr
    struct.pack_into('<H', entry, 20, (cluster >> 16) & 0xFFFF)
    struct.pack_into('<H', entry, 26, cluster & 0xFFFF)
    struct.pack_into('<I', entry, 28, size)
    return bytes(entry)


class DirState:
    def __init__(self, cluster: int) -> None:
        self.cluster = cluster
        self.clusters = [cluster]
        self.next_index = 0
        self.short_names = set()


class Fat32Image:
    def __init__(self, f, part_lba: int, part_sectors: int, bytes_per_sector: int,
                 sectors_per_cluster: int, reserved_sectors: int, num_fats: int, sectors_per_fat: int) -> None:
        self.f = f
        self.part_lba = part_lba
        self.part_sectors = part_sectors
        self.bytes_per_sector = bytes_per_sector
        self.sectors_per_cluster = sectors_per_cluster
        self.reserved_sectors = reserved_sectors
        self.num_fats = num_fats
        self.sectors_per_fat = sectors_per_fat
        self.fat_lba = part_lba + reserved_sectors
        self.data_lba = self.fat_lba + num_fats * sectors_per_fat
        data_sectors = part_sectors - reserved_sectors - num_fats * sectors_per_fat
        self.cluster_count = data_sectors // sectors_per_cluster
        self.cluster_size = bytes_per_sector * sectors_per_cluster
        self.fat = [0] * (self.cluster_count + 2)
        self.fat[0] = 0x0FFFFFF8
        self.fat[1] = 0x0FFFFFFF
        self.alloc_hint = 2

    def cluster_offset(self, cluster: int) -> int:
        return (self.data_lba * SECTOR_SIZE) + (cluster - 2) * self.cluster_size

    def zero_cluster(self, cluster: int) -> None:
        zero = b"\x00" * self.cluster_size
        write_at(self.f, self.cluster_offset(cluster), zero)

    def alloc_cluster(self) -> int:
        for c in range(self.alloc_hint, len(self.fat)):
            if self.fat[c] == 0:
                self.fat[c] = FAT32_EOC
                self.alloc_hint = c + 1
                self.zero_cluster(c)
                return c
        raise RuntimeError("no free cluster")

    def write_fat(self) -> None:
        fat_bytes = bytearray(len(self.fat) * 4)
        for i, val in enumerate(self.fat):
            struct.pack_into('<I', fat_bytes, i * 4, val)
        for fat in range(self.num_fats):
            off = (self.fat_lba + fat * self.sectors_per_fat) * SECTOR_SIZE
            write_at(self.f, off, fat_bytes)

    def ensure_dir_capacity(self, state: DirState, entry_index: int) -> None:
        entries_per_cluster = self.cluster_size // 32
        cluster_idx = entry_index // entries_per_cluster
        while cluster_idx >= len(state.clusters):
            new_cluster = self.alloc_cluster()
            last = state.clusters[-1]
            self.fat[last] = new_cluster
            self.fat[new_cluster] = FAT32_EOC
            state.clusters.append(new_cluster)

    def write_dir_entry(self, state: DirState, entry_index: int, entry: bytes) -> None:
        self.ensure_dir_capacity(state, entry_index)
        entries_per_cluster = self.cluster_size // 32
        cluster_idx = entry_index // entries_per_cluster
        offset_in_cluster = (entry_index % entries_per_cluster) * 32
        cluster = state.clusters[cluster_idx]
        write_at(self.f, self.cluster_offset(cluster) + offset_in_cluster, entry)

    def make_short_name(self, name: str, used: set) -> (bytes, bool):
        dot = name.rfind('.')
        base_src = name if dot < 0 else name[:dot]
        ext_src = '' if dot < 0 else name[dot + 1:]

        base = ''
        ext = ''
        needs_lfn = False
        simple = True

        if len(base_src) == 0:
            base = '_'
            needs_lfn = True

        for c in base_src:
            if 'a' <= c <= 'z':
                c = c.upper()
                needs_lfn = True
            if not is_short_char(c):
                c = '_'
                needs_lfn = True
            if len(base) < 8:
                base += c
            else:
                needs_lfn = True
                simple = False

        for c in ext_src:
            if 'a' <= c <= 'z':
                c = c.upper()
                needs_lfn = True
            if not is_short_char(c):
                c = '_'
                needs_lfn = True
            if len(ext) < 3:
                ext += c
            else:
                needs_lfn = True
                simple = False

        if len(base_src) > 8 or len(ext_src) > 3:
            needs_lfn = True
            simple = False

        short = (base.ljust(8) + ext.ljust(3)).encode('ascii')
        if simple and short not in used:
            return short, needs_lfn

        needs_lfn = True
        base_clean = base if base else '_'
        for n in range(1, 100):
            suffix = f"~{n}"
            keep = 8 - len(suffix)
            if keep < 1:
                keep = 1
            tmp = (base_clean[:keep] + suffix).ljust(8)
            short = (tmp + ext.ljust(3)).encode('ascii')
            if short not in used:
                return short, needs_lfn
        raise RuntimeError("short name collision")

    def add_entry(self, state: DirState, name: str, attr: int, cluster: int, size: int) -> None:
        ensure_ascii_name(name)
        short, needs_lfn = self.make_short_name(name, state.short_names)
        state.short_names.add(short)
        entries = []
        if needs_lfn:
            checksum = lfn_checksum(short)
            lfn_count = (len(name) + 12) // 13
            for i in range(lfn_count):
                ord_idx = lfn_count - i
                if i == 0:
                    ord_idx |= 0x40
                entries.append(build_lfn_entry(name, ord_idx, checksum))
        entries.append(build_short_entry(short, attr, cluster, size))

        for entry in entries:
            self.write_dir_entry(state, state.next_index, entry)
            state.next_index += 1

    def write_dot_entries(self, state: DirState, cluster: int, parent_cluster: int) -> None:
        dot = build_short_entry(b".          ", FAT_ATTR_DIR, cluster, 0)
        dotdot = build_short_entry(b"..         ", FAT_ATTR_DIR, parent_cluster, 0)
        self.write_dir_entry(state, 0, dot)
        self.write_dir_entry(state, 1, dotdot)
        state.next_index = 2

    def mkdir(self, parent: DirState, name: str, parent_cluster: int) -> DirState:
        cluster = self.alloc_cluster()
        state = DirState(cluster)
        self.write_dot_entries(state, cluster, parent_cluster)
        self.add_entry(parent, name, FAT_ATTR_DIR, cluster, 0)
        return state

    def add_file(self, parent: DirState, name: str, data: bytes) -> None:
        if not data:
            self.add_entry(parent, name, FAT_ATTR_ARCHIVE, 0, 0)
            return
        need = (len(data) + self.cluster_size - 1) // self.cluster_size
        first = 0
        prev = 0
        for i in range(need):
            cluster = self.alloc_cluster()
            if first == 0:
                first = cluster
            if prev != 0:
                self.fat[prev] = cluster
            prev = cluster
            chunk = data[i * self.cluster_size:(i + 1) * self.cluster_size]
            write_at(self.f, self.cluster_offset(cluster), chunk)
        self.fat[prev] = FAT32_EOC
        self.add_entry(parent, name, FAT_ATTR_ARCHIVE, first, len(data))


def format_fat32(path: str, size_bytes: int, root_dir: str) -> None:
    part_lba = 2048
    if size_bytes % SECTOR_SIZE != 0:
        raise ValueError("size must be a multiple of 512 bytes")
    total_sectors = size_bytes // SECTOR_SIZE
    if total_sectors <= part_lba + 1024:
        raise ValueError("size too small for FAT32")

    part_sectors = total_sectors - part_lba

    bytes_per_sector = SECTOR_SIZE
    sectors_per_cluster = 8
    reserved_sectors = 32
    num_fats = 2

    sectors_per_fat = compute_fat_size(part_sectors, reserved_sectors, num_fats, sectors_per_cluster)

    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.truncate(size_bytes)

        mbr = bytearray(SECTOR_SIZE)
        part_entry = struct.pack(
            "<B3sB3sII",
            0x00,
            b"\x00\x00\x00",
            PARTITION_TYPE_FAT32,
            b"\x00\x00\x00",
            part_lba,
            part_sectors,
        )
        mbr[446:446 + 16] = part_entry
        struct.pack_into("<H", mbr, 510, MBR_SIGNATURE)
        write_at(f, 0, mbr)

        bpb = bytearray(SECTOR_SIZE)
        bpb[0:3] = b"\xEB\x58\x90"
        bpb[3:11] = b"MKFSFAT "
        struct.pack_into("<H", bpb, 11, bytes_per_sector)
        bpb[13] = sectors_per_cluster
        struct.pack_into("<H", bpb, 14, reserved_sectors)
        bpb[16] = num_fats
        struct.pack_into("<H", bpb, 17, 0)
        struct.pack_into("<H", bpb, 19, 0)
        bpb[21] = 0xF8
        struct.pack_into("<H", bpb, 22, 0)
        struct.pack_into("<H", bpb, 24, 63)
        struct.pack_into("<H", bpb, 26, 255)
        struct.pack_into("<I", bpb, 28, part_lba)
        struct.pack_into("<I", bpb, 32, part_sectors)
        struct.pack_into("<I", bpb, 36, sectors_per_fat)
        struct.pack_into("<H", bpb, 40, 0)
        struct.pack_into("<H", bpb, 42, 0)
        struct.pack_into("<I", bpb, 44, 2)
        struct.pack_into("<H", bpb, 48, 1)
        struct.pack_into("<H", bpb, 50, 6)
        bpb[64] = 0x80
        bpb[66] = 0x29
        struct.pack_into("<I", bpb, 67, 0x12345678)
        bpb[71:82] = b"NO NAME    "
        bpb[82:90] = b"FAT32   "
        struct.pack_into("<H", bpb, 510, MBR_SIGNATURE)
        write_at(f, part_lba * SECTOR_SIZE, bpb)

        fsinfo = bytearray(SECTOR_SIZE)
        struct.pack_into("<I", fsinfo, 0, 0x41615252)
        struct.pack_into("<I", fsinfo, 484, 0x61417272)
        struct.pack_into("<I", fsinfo, 488, 0xFFFFFFFF)
        struct.pack_into("<I", fsinfo, 492, 0xFFFFFFFF)
        struct.pack_into("<H", fsinfo, 510, MBR_SIGNATURE)
        write_at(f, (part_lba + 1) * SECTOR_SIZE, fsinfo)

        write_at(f, (part_lba + 6) * SECTOR_SIZE, bpb)

        fatimg = Fat32Image(
            f,
            part_lba,
            part_sectors,
            bytes_per_sector,
            sectors_per_cluster,
            reserved_sectors,
            num_fats,
            sectors_per_fat,
        )

        root = DirState(2)
        fatimg.fat[2] = FAT32_EOC
        fatimg.zero_cluster(2)

        ram_src = None
        if root_dir:
            for name in os.listdir(root_dir):
                if name.lower() == "ram":
                    ram_src = os.path.join(root_dir, name)
                    if not os.path.isdir(ram_src):
                        raise ValueError("rootfs has non-directory ram")
                    break

        ram_dir = fatimg.mkdir(root, "ram", 2)
        if ram_src:
            copy_tree(fatimg, ram_dir, ram_src)

        if root_dir:
            copy_tree(fatimg, root, root_dir, skip_name="ram")

        fatimg.write_fat()


def copy_tree(fatimg: Fat32Image, parent: DirState, host_dir: str, skip_name: str = None) -> None:
    names = sorted(os.listdir(host_dir), key=lambda s: s.lower())
    for name in names:
        if name in [".", ".."]:
            continue
        if skip_name and name.lower() == skip_name.lower():
            continue
        ensure_ascii_name(name)
        src = os.path.join(host_dir, name)
        if os.path.isdir(src):
            child = fatimg.mkdir(parent, name, parent.cluster)
            copy_tree(fatimg, child, src)
        else:
            with open(src, "rb") as f:
                data = f.read()
            fatimg.add_file(parent, name, data)


def main() -> int:
    parser = argparse.ArgumentParser(description="Create a minimal FAT32 image with an MBR.")
    parser.add_argument("--out", required=True, help="output image path")
    parser.add_argument("--size", default="128M", help="image size, e.g. 128M")
    parser.add_argument("--root", help="host directory to copy into the root of the image")
    parser.add_argument("--force", action="store_true", help="overwrite existing image")
    args = parser.parse_args()

    if os.path.exists(args.out) and not args.force:
        print(f"mkfs: {args.out} exists, skipping format")
        return 0

    root_dir = None
    if args.root:
        root_dir = os.path.abspath(args.root)
        if not os.path.isdir(root_dir):
            raise ValueError(f"root dir not found: {root_dir}")

    size_bytes = parse_size(args.size)
    format_fat32(args.out, size_bytes, root_dir)
    print(f"mkfs: created {args.out} ({size_bytes} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
