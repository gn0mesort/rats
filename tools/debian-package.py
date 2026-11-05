#!/usr/bin/env python3

from argparse import ArgumentParser
from pathlib import Path
import subprocess

def elves(path: Path) -> list[Path]:
    res = [ ]
    for possible in path.rglob("*"):
        if possible.is_file():
            with open(possible, "rb") as file:
                if file.read(4) == b"\x7fELF":
                    res.append(f"-e{possible.absolute()}")
    return res

def main() -> None:
    try:
        parser = ArgumentParser(prog="debian-package.py")
        parser.add_argument("STAGING", type=Path)
        args = parser.parse_args()
        selfdir = Path(__file__).parent.absolute()
        packagingdir = (selfdir.parent / "packaging/debian").absolute()
        builddir = (selfdir.parent / "build").absolute()
        stagedir = args.STAGING.absolute()
        binaries = elves(stagedir / "usr/libexec")
        (builddir / "substvars").touch()
        (stagedir / "DEBIAN").mkdir(exist_ok=True)
        subprocess.run([ "dpkg-shlibdeps", f"-O{builddir / "substvars"}", *binaries ], check=True,
                       cwd=(selfdir.parent / "packaging").absolute())
        subprocess.run([ "dpkg-gencontrol", f"-c{packagingdir / "control"}", f"-l{packagingdir / "changelog"}",
                         f"-P{stagedir}", f"-T{builddir / "substvars"}", f"-f{builddir / "debian-files"}",
                         f"-O{stagedir / "DEBIAN/control"}" ], check=True)
        outdir = Path("dist").absolute()
        outdir.mkdir(exist_ok=True)
        subprocess.run([ "dpkg-deb", "--build", "--root-owner-group", stagedir, outdir ], check=True)
    except Exception as err:
        print(err)
        exit(1)

if __name__ == "__main__":
    main()
