#!/usr/bin/env python3
import sys
from pathlib import Path

def main() -> None:
    path = Path(sys.argv[1]).absolute()
    with open(path, "r") as file:
        print(file.read())

if __name__ == "__main__":
    main()
