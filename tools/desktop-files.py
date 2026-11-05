#!/usr/bin/env python3
from argparse import ArgumentParser
from pathlib import Path

def main() -> None:
    parser = ArgumentParser(prog="desktop-files.py")
    parser.add_argument("INPUT", type=Path)
    parser.add_argument("OUTPUT", type=Path)
    parser.add_argument("BINPATH", type=Path)
    parser.add_argument("EXECPATH", type=str)
    parser.add_argument("VERSION", type=str)
    args = parser.parse_args()
    pattern = ""
    with open(args.INPUT, "rb") as file:
        pattern = file.read().decode("utf-8")
    generated = pattern.format(binpath=args.BINPATH, execpath=args.EXECPATH, version=args.VERSION)
    with open(args.OUTPUT, "wb") as file:
        file.write(generated.encode("utf-8"))

if __name__ == "__main__":
    main()
