#!/usr/bin/env python3

import argparse
import zlib

def main(args):
    if args.fixed:
        strategy = zlib.Z_FIXED
    else:
        strategy = zlib.Z_DEFAULT_STRATEGY
    compression = zlib.compressobj(
        # negative means no header or trailing checksum.
        # basically raw DEFLATE instead of zlib format.
        wbits=-zlib.MAX_WBITS,
        strategy=strategy)
    with open(args.src, "rb") as src:
        data = src.read()
    compressed = compression.compress(data)
    compressed += compression.flush()
    dst = args.src
    dst += ".fixed" if args.fixed else ".dynamic"
    dst += ".deflate"
    print("writing to", dst)
    with open(dst, "wb") as dst:
        dst.write(compressed)

parser = argparse.ArgumentParser(
    prog="gen_test_data",
    description="generates deflate compressed data from a file",
)

parser.add_argument("--src", help="path to input file")
parser.add_argument("--fixed", help="use fixed strategy", action="store_true")

if __name__ == "__main__":
    main(parser.parse_args())
