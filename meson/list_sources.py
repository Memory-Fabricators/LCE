#!/usr/bin/env python3
"""Enumerate source files for a meson build target, mirroring the
file(GLOB...) / file(GLOB_RECURSE...) / list(FILTER EXCLUDE REGEX...) /
list(REMOVE_ITEM...) source selection previously done in CMakeLists.txt.

Usage:
    list_sources.py <root-dir> <mode> [--subdir DIR] [--ext EXT ...]
                     [--exclude REGEX ...] [--remove PATH ...]

    <root-dir> is the target's source root (e.g. .../Minecraft.Client).
    <mode> is either "flat" (like file(GLOB)) or "recursive" (like
    file(GLOB_RECURSE)).
    --subdir restricts the glob to a subdirectory of <root-dir> (e.g.
    "Common" or "SDL3"); omit for the root itself.
    --exclude regexes are matched (like CMake's list(FILTER EXCLUDE REGEX))
    against the path relative to <root-dir>, prefixed with "/" so patterns
    such as "/Common/UI/" behave identically to the original CMake filters.
    --remove paths are matched against the path relative to <root-dir>.

Prints one path per line, relative to <root-dir>, using forward slashes,
sorted for reproducibility.
"""

import argparse
import re
import sys
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path)
    parser.add_argument("mode", choices=["flat", "recursive"])
    parser.add_argument("--subdir", default="")
    parser.add_argument("--ext", action="append", default=[])
    parser.add_argument("--exclude", action="append", default=[])
    parser.add_argument("--remove", action="append", default=[])
    args = parser.parse_args()

    root = args.root.resolve()
    search_dir = (root / args.subdir) if args.subdir else root
    exts = args.ext or ["cpp"]
    exclude_res = [re.compile(pattern) for pattern in args.exclude]
    remove_set = {str(Path(p)) for p in args.remove}

    paths = []
    if search_dir.is_dir():
        for ext in exts:
            glob_pattern = f"*.{ext}" if args.mode == "flat" else f"**/*.{ext}"
            for path in search_dir.glob(glob_pattern):
                if not path.is_file():
                    continue
                rel = path.relative_to(root).as_posix()
                if any(pattern.search("/" + rel) for pattern in exclude_res):
                    continue
                if rel in remove_set:
                    continue
                paths.append(rel)

    for rel in sorted(paths):
        print(rel)

    return 0


if __name__ == "__main__":
    sys.exit(main())
