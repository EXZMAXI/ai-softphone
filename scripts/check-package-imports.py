#!/usr/bin/env python3
#
# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

"""Holds every import between a Windows package's own modules to an export.

A Windows tree is flat and resolved by name: a plugin asks for
``softphone_core.dll`` and gets whichever copy the loader finds, and nothing
checks that the copy it finds is the one the plugin was compiled against. When
it is not, what happens is not a missing feature — it is the operating system's
own message box, naming a mangled C++ symbol, over a screen with no phone on it.

Reported on 2026-10-13 from an installation in that state. Saying that the
*published* packages were whole took an hour of unpacking two installers by hand
and comparing 33 117 symbols; this is that hour written down, and it runs in
seconds.

**Every module, not every module one run happens to load.** Starting the phone
and reading what it refused is the other half of the same question and covers
only the plugins that start; this reads the files, so a plugin for a module
somebody has switched off is read exactly as carefully as one that is always on.

What it does *not* do is read outside the tree: a module naming a library
Windows supplies, or one the package deliberately leaves to the machine, is left
alone. The question is only whether the files shipped together agree with each
other.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

BINARY_SUFFIXES = (".dll", ".exe")

class NotAPeFile(Exception):
    """The file is not a Windows binary this can read — a resource, a stub."""

class Binary:
    """Just enough of a PE image to answer *what does it ask for, and of whom*.

    Written here rather than taken from `dumpbin`, which is Visual Studio's and
    therefore absent from any machine that did not build this, and which answers
    in whatever language that installation speaks — a check that parses somebody
    else's prose is a check that passes on a translated machine.
    """

    def __init__(self, path: Path) -> None:
        self.path = path
        self.data = path.read_bytes()
        self._sections: list[tuple[int, int, int]] = []
        self._read_headers()

    def _read_headers(self) -> None:
        data = self.data
        if len(data) < 0x40 or data[:2] != b"MZ":
            raise NotAPeFile(f"{self.path.name} does not begin with MZ")

        at = struct.unpack_from("<I", data, 0x3C)[0]
        if at + 24 > len(data) or data[at : at + 4] != b"PE\0\0":
            raise NotAPeFile(f"{self.path.name} has no PE header")

        sections = struct.unpack_from("<H", data, at + 6)[0]
        optional_size = struct.unpack_from("<H", data, at + 20)[0]

        optional_at = at + 24
        magic = struct.unpack_from("<H", data, optional_at)[0]
        if magic == 0x20B:  # PE32+
            self.wide = True
            directories_at = optional_at + 112
        elif magic == 0x10B:  # PE32
            self.wide = False
            directories_at = optional_at + 96
        else:
            raise NotAPeFile(f"{self.path.name} has an optional header this cannot read")

        self.directories: list[tuple[int, int]] = []
        count = struct.unpack_from("<I", data, directories_at - 4)[0]
        for index in range(min(count, 16)):
            self.directories.append(struct.unpack_from("<II", data, directories_at + index * 8))

        table_at = optional_at + optional_size
        for index in range(sections):
            entry = table_at + index * 40
            virtual_size, virtual_address, raw_size, raw_at = struct.unpack_from(
                "<IIII", data, entry + 8
            )
            self._sections.append((virtual_address, max(virtual_size, raw_size), raw_at))

    def _offset(self, rva: int) -> int | None:
        """Where an address the image uses lands in the file, or nothing."""
        for virtual_address, size, raw_at in self._sections:
            if virtual_address <= rva < virtual_address + size:
                return raw_at + (rva - virtual_address)
        return None

    def _text(self, rva: int) -> str | None:
        at = self._offset(rva)
        if at is None or at >= len(self.data):
            return None
        end = self.data.find(b"\0", at)
        if end < 0:
            return None
        return self.data[at:end].decode("latin-1")

    def exports(self) -> set[str]:
        """Every name this module offers. Ordinals are not names and are left out.

        Nothing in this product imports by ordinal — `CMAKE_WINDOWS_EXPORT_ALL_
        SYMBOLS` writes names — so a module that did would be read as asking for
        nothing, which is why `imports` counts those separately.
        """
        if len(self.directories) < 1:
            return set()
        rva, size = self.directories[0]
        if rva == 0 or size == 0:
            return set()
        at = self._offset(rva)
        if at is None:
            return set()

        count, names_rva = struct.unpack_from("<I", self.data, at + 24)[0], struct.unpack_from(
            "<I", self.data, at + 32
        )[0]
        found: set[str] = set()
        table = self._offset(names_rva)
        if table is None:
            return found
        for index in range(count):
            if table + index * 4 + 4 > len(self.data):
                break
            name = self._text(struct.unpack_from("<I", self.data, table + index * 4)[0])
            if name is not None:
                found.add(name)
        return found

    def imports(self) -> dict[str, tuple[set[str], int]]:
        """What it asks of each module it names: the names, and how many ordinals."""
        asked: dict[str, tuple[set[str], int]] = {}
        if len(self.directories) < 2:
            return asked
        rva, size = self.directories[1]
        if rva == 0 or size == 0:
            return asked
        at = self._offset(rva)
        if at is None:
            return asked

        step = 8 if self.wide else 4
        high = 1 << (63 if self.wide else 31)
        shape = "<Q" if self.wide else "<I"

        entry = at
        while entry + 20 <= len(self.data):
            lookup, _, _, name_rva, address = struct.unpack_from("<IIIII", self.data, entry)
            if lookup == 0 and name_rva == 0 and address == 0:
                break
            entry += 20

            module = self._text(name_rva)
            if module is None:
                continue
            names: set[str] = set()
            ordinals = 0

            thunks = self._offset(lookup or address)
            if thunks is not None:
                while thunks + step <= len(self.data):
                    value = struct.unpack_from(shape, self.data, thunks)[0]
                    thunks += step
                    if value == 0:
                        break
                    if value & high:
                        ordinals += 1
                        continue
                    name = self._text((value & 0x7FFFFFFF) + 2)
                    if name is not None:
                        names.add(name)

            known, counted = asked.get(module.lower(), (set(), 0))
            asked[module.lower()] = (known | names, counted + ordinals)

        return asked

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tree", type=Path, help="the staged or installed tree to read")
    options = parser.parse_args()

    tree: Path = options.tree
    if not tree.is_dir():
        print(f"check-package-imports: {tree} is not a directory", file=sys.stderr)
        return 1

    modules: dict[str, Path] = {}
    for path in sorted(tree.rglob("*")):
        if path.is_file() and path.suffix.lower() in BINARY_SUFFIXES:
            modules.setdefault(path.name.lower(), path)

    if not modules:
        print(f"check-package-imports: no .dll or .exe under {tree}", file=sys.stderr)
        return 1

    exports: dict[str, set[str]] = {}
    checked = 0
    problems: list[str] = []
    unreadable: list[str] = []

    for name, path in modules.items():
        try:
            binary = Binary(path)
        except NotAPeFile as why:
            unreadable.append(str(why))
            continue

        for named, (wanted, _ordinals) in binary.imports().items():
            if named not in modules:
                continue  # Windows's own, or a library the machine supplies.
            if named not in exports:
                try:
                    exports[named] = Binary(modules[named]).exports()
                except NotAPeFile:
                    exports[named] = set()
            for symbol in sorted(wanted):
                checked += 1
                if symbol not in exports[named]:
                    problems.append(
                        f"{path.relative_to(tree)} asks {named} for {symbol}, "
                        f"which that file does not offer"
                    )

    for why in unreadable:
        print(f"  note  {why}")

    if problems:
        for problem in problems[:20]:
            print(problem)
        if len(problems) > 20:
            print(f"and {len(problems) - 20} more")
        print(
            "check-package-imports: this tree's files are not all from one build. "
            "What a person sees is the operating system's own message box and no phone."
        )
        return 1

    if checked == 0:
        print(
            "check-package-imports: nothing in this tree asks anything of anything "
            "else in it, which is not an answer this check can give about a package",
            file=sys.stderr,
        )
        return 1

    print(
        f"  ok    {checked} symbol(s) asked between the {len(modules)} modules of this "
        f"tree, and every one is offered"
    )
    return 0

if __name__ == "__main__":
    sys.exit(main())
