#!/usr/bin/env python3
"""Regenerate vw-hud/_hudmath.h from its own marked canonical blocks.

NO AI, NO network, stdlib only. The canonical helper definitions live between
`// HUDMATH-BEGIN <name>` and `// HUDMATH-END` markers inside _hudmath.h itself.
This script:

  1. parses those marked blocks out of the existing _hudmath.h,
  2. re-emits the file = fixed preamble + the blocks (in declared order) + fixed
     epilogue.

It is idempotent: running it twice yields a byte-identical file, because the
output is fully determined by the marked block contents plus the constant
template below. Edit a block, run `python3 tools/gen_hudmath.py`, done.

Usage:
    python3 tools/gen_hudmath.py            # rewrite vw-hud/_hudmath.h in place
    python3 tools/gen_hudmath.py --check    # exit 1 if the file is not normalised
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
HEADER = os.path.normpath(os.path.join(HERE, "..", "vw-hud", "_hudmath.h"))

BEGIN_RE = re.compile(r"^// HUDMATH-BEGIN (\S+)\s*$")
END_RE = re.compile(r"^// HUDMATH-END\s*$")

PREAMBLE = """\
#ifndef _HUDMATH_H_
#define _HUDMATH_H_

// =============================================================================
// _hudmath.h - pure display-limit math for the VW-PQ HUD
// =============================================================================
//
// Dependency-free numeric helpers extracted from Hudisplay.cpp /
// Hudisplay128x64.cpp. NO Arduino.h / U8g2 required: only <stdint.h>, so this
// header compiles both in the firmware and natively under the test/ shim.
//
// All functions are `static inline`, branch-light, and integer where the
// originals are integer. They reproduce the exact arithmetic that used to be
// inlined in the drawing/stats code so on-screen behaviour is unchanged.
//
// GENERATED REGION: the bodies between the `// HUDMATH-BEGIN <name>` and
// `// HUDMATH-END` markers are the canonical source of truth. tools/gen_hudmath.py
// re-assembles this file from those marked blocks (idempotent). Edit the marked
// blocks here, then run `python3 tools/gen_hudmath.py` to normalise the file.
// See tools/README.md (no AI required).
// =============================================================================

#include <stdint.h>
"""

EPILOGUE = """\
#endif // _HUDMATH_H_
"""


def parse_blocks(text):
    """Return [(name, block_text_including_markers), ...] in file order."""
    blocks = []
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        m = BEGIN_RE.match(lines[i])
        if not m:
            i += 1
            continue
        name = m.group(1)
        start = i
        i += 1
        while i < len(lines) and not END_RE.match(lines[i]):
            i += 1
        if i >= len(lines):
            sys.exit("error: HUDMATH-BEGIN %s without matching HUDMATH-END" % name)
        block = "\n".join(lines[start:i + 1])  # inclusive of END marker
        blocks.append((name, block))
        i += 1
    if not blocks:
        sys.exit("error: no HUDMATH-BEGIN/END blocks found in %s" % HEADER)
    return blocks


def render(blocks):
    parts = [PREAMBLE]
    for _name, block in blocks:
        parts.append("")  # blank line before each block
        parts.append(block)
    parts.append("")  # blank line before epilogue
    parts.append(EPILOGUE)
    return "\n".join(parts).rstrip("\n") + "\n"


def main():
    check = "--check" in sys.argv[1:]
    with open(HEADER, "r") as f:
        current = f.read()
    blocks = parse_blocks(current)
    out = render(blocks)
    if check:
        if out != current:
            sys.exit("error: %s is not normalised; run python3 tools/gen_hudmath.py" % HEADER)
        print("ok: _hudmath.h is normalised (%d blocks)" % len(blocks))
        return
    if out != current:
        with open(HEADER, "w") as f:
            f.write(out)
        print("wrote %s (%d blocks)" % (HEADER, len(blocks)))
    else:
        print("unchanged %s (%d blocks)" % (HEADER, len(blocks)))


if __name__ == "__main__":
    main()
