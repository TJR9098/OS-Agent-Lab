#!/usr/bin/env python3
import argparse
import sys
import time


def main() -> int:
    parser = argparse.ArgumentParser(description="Feed stdin with line delays.")
    parser.add_argument("--input", required=True, help="Input text file")
    parser.add_argument("--delay", type=float, default=0.2, help="Delay between lines (seconds)")
    args = parser.parse_args()

    with open(args.input, "r", newline="") as f:
        for line in f:
            sys.stdout.write(line)
            sys.stdout.flush()
            if args.delay > 0:
                time.sleep(args.delay)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
