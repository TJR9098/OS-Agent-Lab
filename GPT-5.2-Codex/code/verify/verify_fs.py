#!/usr/bin/env python3
import argparse
import struct

SECTOR_SIZE = 512


def read_at(f, offset: int, size: int) -> bytes:
    f.seek(offset)
    return f.read(size)


def le16(buf: bytes) -> int:
    return struct.unpack_from("<H", buf, 0)[0]


def le32(buf: bytes) -> int:
    return struct.unpack_from("<I", buf, 0)[0]


def dump(image_path: str) -> None:
    with open(image_path, "rb") as f:
        mbr = read_at(f, 0, SECTOR_SIZE)
        mbr_sig = le16(mbr[510:512])
        part = mbr[446:446 + 16]
        part_type = part[4]
        lba_start = le32(part[8:12])
        sectors = le32(part[12:16])
        print(f"MBR_SIG=0x{mbr_sig:04x} TYPE=0x{part_type:02x} LBA_START={lba_start} SECTORS={sectors}")

        bpb = read_at(f, lba_start * SECTOR_SIZE, SECTOR_SIZE)
        bpb_sig = le16(bpb[510:512])
        bytes_per_sector = le16(bpb[11:13])
        sectors_per_cluster = bpb[13]
        reserved = le16(bpb[14:16])
        fats = bpb[16]
        spf = le16(bpb[22:24])
        if spf == 0:
            spf = le32(bpb[36:40])
        root = le32(bpb[44:48])
        data_lba = lba_start + reserved + fats * spf
        print(
            "BPB_SIG=0x{sig:04x} BPS={bps} SPC={spc} RESERVED={res} FATS={fats} SPF={spf} ROOT={root} DATA_LBA={data}".format(
                sig=bpb_sig,
                bps=bytes_per_sector,
                spc=sectors_per_cluster,
                res=reserved,
                fats=fats,
                spf=spf,
                root=root,
                data=data_lba,
            )
        )

        root_entry = read_at(f, data_lba * SECTOR_SIZE, 32)
        name = root_entry[0:11].decode("ascii", errors="replace")
        attr = root_entry[11]
        clus_hi = le16(root_entry[20:22])
        clus_lo = le16(root_entry[26:28])
        cluster = (clus_hi << 16) | clus_lo
        print(f"ROOT_ENTRY_NAME={name} ATTR=0x{attr:02x} CLUSTER={cluster}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify FAT32 MBR/BPB/root entry")
    parser.add_argument("--image", default="build/fs_test.img", help="image path")
    args = parser.parse_args()
    dump(args.image)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
