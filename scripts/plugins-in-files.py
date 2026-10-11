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

"""Prints the identifier of every plugin that ships as a file, one a line.

**Three lists by hand were two too many, and the third was the one that
mattered.** `scripts/check-package-tree.sh` kept the set twice — once for the
Linux tree and once for the bundle — and `check-package-tree.ps1` a third time,
and each of them said in its own comment that a new plugin has to be added to
it. On 2026-10-09 the first macOS package cut from the Phase 2 branch would not
start at all, because `realtime` was linked by the shell and its install name
could only be resolved by a reader standing in the plugin tree. The check that
exists to find exactly that approved the package without ever mentioning the
plugin, because none of the three lists had heard of it — which is what the
Linux list's own comment already recorded about `capture` and `0.3.4`, one
release late.

So the set is derived from the one place that decides it: a directory under
`plugins/` whose `CMakeLists.txt` calls `softphone_plugin_in_a_file` is a plugin
that ships as a pair of files and a manifest, and nothing else is. A plugin
added tomorrow is checked tomorrow, in all three trees, without anybody
remembering.

**And it fails rather than printing nothing**, which is this repository's
oldest question asked of a check: what does this look like when it examines
nothing at all, and can I tell that from a pass? An empty answer here would
turn every loop that reads it into a loop that runs no checks and approves of
everything by silence.
"""

from __future__ import annotations

import sys
from pathlib import Path

DECLARATION = "softphone_plugin_in_a_file"

def plugins(root: Path) -> list[str]:
    found = []
    for build in sorted(root.glob("plugins/*/CMakeLists.txt")):
        if DECLARATION in build.read_text(encoding="utf-8"):
            found.append(build.parent.name)
    return found

def main() -> int:
    root = Path(__file__).resolve().parent.parent
    found = plugins(root)
    if not found:
        print(
            f"plugins-in-files: no plugin under {root / 'plugins'} calls "
            f"{DECLARATION}(), so whatever asked has nothing to check.",
            file=sys.stderr,
        )
        return 1

    print("\n".join(found))
    return 0

if __name__ == "__main__":
    sys.exit(main())
