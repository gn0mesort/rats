#!/usr/bin/env python3
import sys
from datetime import datetime, UTC
import subprocess

def main() -> None:
    text = None
    with open(sys.argv[1], "rb") as file:
        text = file.read().decode("utf-8")
    branch = None
    try:
        res = subprocess.run([ "git", "rev-parse", "--abbrev-ref", "HEAD" ], capture_output=True)
        if res.stdout:
            branch = f"\"{res.stdout.decode("utf-8").strip()}\""
    except FileNotFoundError:
        pass
    now = datetime.now(UTC)
    buildtime = f"\"{now.year:04}{now.month:02}{now.day:02}{now.hour:02}{now.minute:02}{now.second:02}\""
    if branch:
        text = text.replace("#mesondefine CONFIG_MESON_PROJECT_VERSION_LABEL",
                            f"#define CONFIG_MESON_PROJECT_VERSION_LABEL {branch}")
    else:
        text = text.replace("#mesondefine CONFIG_MESON_PROJECT_VERSION_LABEL",
                            "/* #undef CONFIG_MESON_PROJECT_VERSION_LABEL */")
    text = text.replace("#mesondefine CONFIG_MESON_PROJECT_VERSION_BUILD",
                        f"#define CONFIG_MESON_PROJECT_VERSION_BUILD {buildtime}")
    with open(sys.argv[2], "wb") as file:
        file.write(text.encode("utf-8"))

if __name__ == "__main__":
    main()
