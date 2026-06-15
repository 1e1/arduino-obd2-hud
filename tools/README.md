# tools/

## `gen_hudmath.py` — regenerate `vw-hud/_hudmath.h`

`_hudmath.h` holds the pure display-limit math extracted from the U8g2-coupled
`Hudisplay` code (fuel-gauge px, consumption ring scale, RPM px, label offsets,
average speed/consumption, µl counter delta). The canonical helper definitions
live **inside `_hudmath.h` itself**, between marker comments:

```c
// HUDMATH-BEGIN <name>
... helper body ...
// HUDMATH-END
```

`gen_hudmath.py` parses those marked blocks and re-emits the whole header as a
fixed preamble + the blocks (in declared order) + a fixed epilogue. The output
is fully determined by the block contents plus the constant template in the
script, so it is **idempotent**: running it twice produces a byte-identical
file.

This is a **plain Python 3, stdlib-only, NO-AI, NO-network** code generator. It
does not call any model or external service.

### Run it

```sh
python3 tools/gen_hudmath.py          # normalise vw-hud/_hudmath.h in place
python3 tools/gen_hudmath.py --check  # CI mode: exit 1 if not normalised
```

### Workflow for editing the math

1. Edit the body inside the relevant `// HUDMATH-BEGIN <name>` / `// HUDMATH-END`
   block in `vw-hud/_hudmath.h`.
2. Run `python3 tools/gen_hudmath.py` to normalise spacing/ordering.
3. Re-run the host unit tests:
   `c++ -std=c++11 -Wall -Itest test/test_hud.cpp -o /tmp/test_hud && /tmp/test_hud`

### Verifying the extraction

The replayable verification is the host unit-test suite `test/test_hud.cpp`,
which exercises the boundary/limit behaviour of every helper. It compiles with
no Arduino/U8g2 dependency (uses the `test/Arduino.h` shim's `<cstdint>`).
