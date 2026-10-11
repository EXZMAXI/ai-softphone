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

"""The corrections this product carries against pjproject's own source.

**One patcher for both build scripts**, because the same edit written twice in
`build-pjproject.sh` and `build-pjproject.ps1` is the same edit that drifts. It
runs against the freshly extracted tree, in the place those scripts already
write `config_site.h` into — so it meets a pristine copy every time and never
has to be idempotent.

**It refuses rather than skips.** A correction that quietly fails to apply is
worse than no correction: the build goes green, the library is the one with the
defect in it, and nothing says so until somebody's phone dies again. So every
edit asserts on the exact text it expects and this script exits non-zero the
moment pjproject's own source has moved — which is what should happen when the
pin changes, because the correction has to be read against the new version
before it is trusted.

**And both build scripts fingerprint this file**, along with CI's cache keys, so
that editing it rebuilds pjproject everywhere rather than serving a cached build
of the previous text. `build-pjproject.sh` carries the paragraph explaining why
a fingerprint that stops covering a file is worse than none.

Usage: scripts/patch-pjproject.py <extracted pjproject tree>
"""

import pathlib
import sys

class Correction:
    """One edit, named for what it fixes, with the text it expects."""

    def __init__(self, name: str, where: str, before: str, after: str) -> None:
        self.name = name
        self.where = where
        self.before = before
        self.after = after

_DIALOG_EVENT = "pjsip/src/pjsip-simple/dlg_event.c"

_LOCAL_BEFORE = """        } else {
            dlgev_st->info[dlgev_st->info_cnt].local_identity.ptr = NULL;
            dlgev_st->info[dlgev_st->info_cnt].local_identity_display.ptr =
                NULL;
            dlgev_st->info[dlgev_st->info_cnt].local_target_uri.ptr = NULL;
        }
"""

_LOCAL_AFTER = """        } else {
            dlgev_st->info[dlgev_st->info_cnt].local_identity.ptr = NULL;
            dlgev_st->info[dlgev_st->info_cnt].local_identity.slen = 0;
            dlgev_st->info[dlgev_st->info_cnt].local_identity_display.ptr =
                NULL;
            dlgev_st->info[dlgev_st->info_cnt].local_identity_display.slen = 0;
            dlgev_st->info[dlgev_st->info_cnt].local_target_uri.ptr = NULL;
            dlgev_st->info[dlgev_st->info_cnt].local_target_uri.slen = 0;
        }
"""

_REMOTE_BEFORE = """        } else {
            dlgev_st->info[dlgev_st->info_cnt].remote_identity.ptr = NULL;
            dlgev_st->info[dlgev_st->info_cnt].remote_identity_display.ptr =
                NULL;
            dlgev_st->info[dlgev_st->info_cnt].remote_target_uri.ptr = NULL;
        }
"""

_REMOTE_AFTER = """        } else {
            dlgev_st->info[dlgev_st->info_cnt].remote_identity.ptr = NULL;
            dlgev_st->info[dlgev_st->info_cnt].remote_identity.slen = 0;
            dlgev_st->info[dlgev_st->info_cnt].remote_identity_display.ptr =
                NULL;
            dlgev_st->info[dlgev_st->info_cnt].remote_identity_display.slen = 0;
            dlgev_st->info[dlgev_st->info_cnt].remote_target_uri.ptr = NULL;
            dlgev_st->info[dlgev_st->info_cnt].remote_target_uri.slen = 0;
        }
"""

CORRECTIONS = [
    Correction(
        "a dialog with no <local> keeps the previous notification's length",
        _DIALOG_EVENT,
        _LOCAL_BEFORE,
        _LOCAL_AFTER,
    ),
    Correction(
        "a dialog with no <remote> keeps the previous notification's length",
        _DIALOG_EVENT,
        _REMOTE_BEFORE,
        _REMOTE_AFTER,
    ),
]

def read(path):
    """The file as it is, with every newline left alone."""
    with open(path, "r", encoding="utf-8", newline="") as handle:
        return handle.read()

def write(path, text):
    """The file back, with every newline left alone."""
    with open(path, "w", encoding="utf-8", newline="") as handle:
        handle.write(text)

def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <extracted pjproject tree>", file=sys.stderr)
        return 2

    tree = pathlib.Path(sys.argv[1])
    if not tree.is_dir():
        print(f"patch-pjproject: {tree} is not a directory", file=sys.stderr)
        return 2

    for correction in CORRECTIONS:
        target = tree / correction.where
        if not target.is_file():
            print(f"patch-pjproject: {target} is not there", file=sys.stderr)
            return 1

        source = read(target)
        found = source.count(correction.before)
        if found != 1:
            print(
                f"patch-pjproject: \"{correction.name}\"\n"
                f"  expected exactly one occurrence in {correction.where}, found {found}.\n"
                "  pjproject's source has moved. Read the correction against the new\n"
                "  version before trusting it — do not relax this check.",
                file=sys.stderr,
            )
            return 1

        write(target, source.replace(correction.before, correction.after))

        if correction.after not in read(target):
            print(f"patch-pjproject: \"{correction.name}\" did not take", file=sys.stderr)
            return 1

        print(f"[pjproject] corrected: {correction.name}")

    return 0

if __name__ == "__main__":
    sys.exit(main())
