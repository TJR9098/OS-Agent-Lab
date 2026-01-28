#!/usr/bin/env python3
import argparse
import sys
import time


def main() -> int:
    parser = argparse.ArgumentParser(description="Feed stdin with line delays.")
    parser.add_argument("--input", required=True, help="Input text file")
    parser.add_argument("--delay", type=float, default=0.2, help="Delay between lines (seconds)")
    parser.add_argument("--raw", action="store_true", help="Send raw bytes instead of line mode")
    parser.add_argument("--byte-delay", type=float, default=0.0, help="Delay between bytes in raw mode")
    args = parser.parse_args()

    if args.raw:
        with open(args.input, "rb") as f:
            data = f.read()
        out = sys.stdout.buffer
        for b in data:
            out.write(bytes([b]))
            out.flush()
            if args.byte_delay > 0:
                time.sleep(args.byte_delay)
        return 0

    with open(args.input, "r", newline="") as f:
        for line in f:
            sys.stdout.write(line)
            sys.stdout.flush()
            if args.delay > 0:
                time.sleep(args.delay)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
