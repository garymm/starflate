#!/usr/bin/env python3

import argparse
import sys
import zlib

def main(args):
    strategy = zlib.Z_FIXED if args.fixed else zlib.Z_DEFAULT_STRATEGY
    compression = zlib.compressobj(
        # negative means no header or trailing checksum.
        # basically raw DEFLATE instead of zlib format.
        wbits=-zlib.MAX_WBITS,
        strategy=strategy)
    with open(args.src, "rb") as src:
        data = src.read()
    compressed = compression.compress(data)
    compressed += compression.flush()

    sys.stdout.buffer.write(compressed)

parser = argparse.ArgumentParser(
    prog="deflate_compress",
    description="generates deflate compressed data from a file",
)

parser.add_argument("--src", help="path to input file", required=True)
parser.add_argument("--fixed", help="use fixed strategy", action="store_true")

if __name__ == "__main__":
    main(parser.parse_args())
