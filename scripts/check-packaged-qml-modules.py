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

"""Every QML module this product imports is in the tree that ships.

`qt_generate_deploy_qml_app_script` runs Qt's import scanner and the scanner is
right: given the shell's QML it reports what the shell imports, and
`import QtQuick.Dialogs` is in `Main.qml` precisely so that a module only the
plugins import would be reported too. What nothing noticed is **when that scan
runs**. Its Ninja rule has exactly one input — `qmlimportscanner.exe` — so the
scan happens once, the first time the file does not exist, and never again for
the life of the build directory. A build directory configured before the import
was written therefore deploys the list it took days ago, and no amount of
building puts that right.

What that costs is the whole phone rather than the feature. Before that import
was written, a missing `QtQuick.Dialogs` meant a plugin's file dialog did not
open; after it, `Main.qml` itself fails to load, the root object is never built,
and the phone exits before its first frame. The message Qt prints names
**`Splash.qml` and a property called `darkWanted`** — a different file, and a
binding whose only crime is being re-evaluated as the context comes apart — so
the message names the wrong file and the wrong property.

**So the question is asked of the staged tree**, which is the only place it can
be answered: every `import` in the product's own QML, against a `qmldir` in the
tree an installer is about to wrap. A stale scan, a scanner that missed a file,
a module a plugin gained yesterday — all three read the same here, because this
reads what ships rather than what a build directory remembers about it.

**And it replaces a list of three written by hand.** `check-package-tree` named
`QtQuick`, `QtQuick.Controls` and `Qt.labs.platform` — three of the ten modules
this product imports, and not the one that broke.

Usage: check-packaged-qml-modules.py <staged tree>
"""

import re
import sys
from pathlib import Path

IMPORT = re.compile(r'^\s*import\s+([A-Za-z_][\w.]*)')

OURS = "Softphone"

def modulesImported(root: Path) -> dict[str, set[str]]:
    """Every module imported by the QML this product ships, and by which file."""
    found: dict[str, set[str]] = {}
    sources = sorted(root.glob("ui/qml/**/*.qml")) + sorted(
        root.glob("plugins/*/qml/**/*.qml"))

    if not sources:
        print("no QML found: this is not the source tree", file=sys.stderr)
        sys.exit(2)

    for qml in sources:
        for line in qml.read_text(encoding="utf-8").splitlines():
            match = IMPORT.match(line)
            if not match:
                continue
            module = match.group(1)
            if module == OURS or module.startswith(OURS + "."):
                continue
            found.setdefault(module, set()).add(
                str(qml.relative_to(root)).replace("\\", "/"))

    return found

def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__.strip().splitlines()[-1], file=sys.stderr)
        return 2

    tree = Path(sys.argv[1])

    roots = [tree / "qml", tree / "usr" / "qml", tree / "lib" / "ai-softphone" / "qml"]
    qmlRoot = next((r for r in roots if (r / "QtQuick" / "qmldir").is_file()), None)

    if qmlRoot is None:
        looked = ", ".join(str(r) for r in roots)
        print(f"no QML modules under any of: {looked}", file=sys.stderr)
        return 2

    root = Path(__file__).resolve().parent.parent
    imported = modulesImported(root)
    missing = []

    for module in sorted(imported):
        where = qmlRoot / Path(*module.split("."))
        if (where / "qmldir").is_file():
            print(f"  ok      {module}")
        else:
            missing.append(module)
            print(f"  MISSING {module}")

    print(f"{len(imported) - len(missing)} of {len(imported)} "
          f"QML modules are in the tree")

    for module in missing:
        askers = sorted(imported[module])
        print(f"\n{module} is imported and is not in the package.")
        print("  imported by: " + ", ".join(askers[:4])
              + (f", and {len(askers) - 4} more" if len(askers) > 4 else ""))
        print("  the phone will not start: a file whose import cannot be "
              "resolved builds no root object, and the message Qt prints names "
              "a different file entirely.")
        if not any(a.startswith("ui/qml/") for a in askers):
            print("  no file in ui/qml imports it, and the deploy scanner reads "
                  "only the shell: add the import to ui/qml/Main.qml, where the "
                  "others are, and say there which plugin wants it.")
        else:
            print("  the shell does import it, so the scan that deployed this "
                  "tree was stale: empty build/package and cut it again.")

    return 1 if missing else 0

if __name__ == "__main__":
    sys.exit(main())
