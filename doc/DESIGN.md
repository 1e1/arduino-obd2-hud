# VW Sharan HUD — Design

A self-built Head-Up Display for a VW Sharan (Sound, TSI 1.5, mid-2018, manual gearbox).

## Overview & goal

- Confirmed platform: **PQ** (late PQ, firmware MST2_EU_VW_PQ_R0604T — not MQB).
- The HUD reads the car's **native PQ CAN** directly and decodes raw frames against `opendbc/vw_pq.dbc`.
- It does **not** use OBD-II PID request/response. OBD-II goes through the gateway, which filters traffic and adds request overhead; native CAN exposes signals OBD cannot give.
- Pipeline across three repos: `discodb2` discovers → `opendbc/vw_pq.dbc` documents → `arduino-obd2-hud` consumes.
- Status: the firmware in `./vw-hud` reads **native PQ CAN**. `CarEventCan` filters by arbitration ID and decodes signals by bit offset (the OBD-II PID backend is gone — no more `_buffer[2]` PID / `0x7E8` filter). It compiles on `arduino:avr:uno` / `arduino:avr:nano` (~67% flash, 46% RAM), and the `.ino` wires the CAN backend as the active source. CAN IDs / bit offsets / scales remain to verify on bench against this car (see "To verify on bench").

## Hardware

- Arduino Nano Every (primary) + external MCP2515 SPI CAN board + SSD1309 128x64 OLED (SPI) + 12V→5V DCC converter.
- Listen-only on the bus. The car already carries 120 Ω at each end (60 Ω total between CAN-H and CAN-L) — do not add a termination resistor.

### Candidate boards

| Board | CPU | ROM | RAM | MCP2515 module |
|---|---|---|---|---|
| Arduino Nano Every (primary) | ATmega4809 @16 MHz | 48 KB | 6 KB | required |
| Arduino Nano classic | ATmega328P @16 MHz | 32 KB | 2 KB (binding constraint) | required |
| Seeed XIAO SAMD21 | Cortex-M0+ @48 MHz | 256 KB | 32 KB | required |
| Seeed XIAO ESP32-C3 | RISC-V @160 MHz | 4 MB | 400 KB | no — native CAN (TWAI), transceiver only |
| Raspberry Pi Pico (RP2040) | 2×Cortex-M0+ @133 MHz | 2 MB | 264 KB | required |
| CANBed RP2040 (Longan) | 2×Cortex-M0+ @133 MHz | 2 MB | 264 KB | no — MCP2515 onboard |

- **Builds today: all six boards compile** across five architectures (classic-AVR, megaavr, SAMD, RP2040, ESP32). `PowerManager` is dual-pathed: classic-AVR keeps the real deep-sleep baseline (watchdog/sleep registers `WDIE`, `WDP0..3`, `avr/wdt.h`, `avr/sleep.h`), every other arch takes a portable path (`VH_DEEP_SLEEP==0`). On that portable path the **RP2040 is no longer a no-op**: `shutdown()` sleeps in `__wfi` and wakes on the MCP2515 INT (bus activity), `standby()` idles in `__wfe`; only the deepest XOSC-dormant (sub-mA) is still pending bench validation. The megaavr / SAMD / ESP32-C3 portable paths are still timed-`delay()` no-ops awaiting their per-arch sleep. The ESP32-C3 additionally uses the `CarEventTwai` backend (no MCP2515).
- The 328P (2 KB RAM) dimensions everything. Two tiers:
  - **constrained** (328P / Nano Every): OLED page mode (`VH_DISPLAY_FULLBUFFER=0`), no full buffer (a full buffer is 1 KB = 50% of 328P RAM).
  - **comfortable** (SAMD21 / RP2040 / ESP32-C3): full buffer + partial `updateDisplayArea`.
- `_wiring.h` now holds a per-architecture pinout block for each board (classic-AVR / megaavr / SAMD / RP2040 / ESP32-C3), keyed on the `ARDUINO_ARCH_*` macros. Pins on the non-AVR blocks are bench-guess except the CANBed RP2040 (confirmed against the Longan + Zephyr board pinout — see below).
- CANBed RP2040 is the most turnkey board: onboard MCP2515 + MCP2551, the existing `CarEventCan` (MCP_CAN lib) works as-is. Its onboard MCP is wired to **SPI0 GP2/3/4, CS GP9, INT GP11, 16 MHz crystal** (confirmed) — NOT the Philhower default SPI0 pins, so the firmware re-routes the bus with `SPI.setSCK/setTX/setRX` in `setup()`. Deep-sleep wakes on the GP11 INT. Validation steps for the whole port are in [RECETTE-CANBED-RP2040.md](./RECETTE-CANBED-RP2040.md).
- ESP32-C3 uses native CAN (TWAI) and needs a new backend `CarEventTwai` + an external transceiver. Native controller opens future options: live telemetry to a discodb2 backend, OTA, BLE config.

### MCP2515 crystal frequency

- The MCP2515 crystal frequency MUST be declared in code — there is no auto-detection. A mismatch with the physical crystal causes silent, total comms failure.
- It is the 3rd argument of `mcp2515.begin(MCP_STD, CAN_500KBPS, MCP_8MHZ)`.
- It varies per module: cheap blue modules are often 8 MHz, sometimes 16 MHz; the CANBed RP2040 onboard controller is 16 MHz.
- It is now a config constant per board: `VH_CAN_CRYSTAL` (+ `VH_CAN_BITRATE`) in `_wiring.h`, passed to `mcp2515.begin()` from the `.ino` (no longer hardcoded inline). CANBed RP2040 = `MCP_16MHZ`.
- Bitrate must match the tapped bus: PQ powertrain = 500 kbps; comfort/Kombi possibly 100 kbps — to verify on bench per the tapped harness.

## Architecture

Clean swappable abstractions, each with backends for bench testing and for the car:

- **CarEvent** — data source. Emits decoded sensor events. Backends: `CarEventStream` (test feed), `CarEventHardware` (test feed), `CarEventCan` (MCP2515), future `CarEventTwai` (ESP32-C3).
- **Hudisplay** — renderer. Holds the display model and the sliding window. Backends: `Hudisplay128x64` (SSD1309 OLED), `HudisplayStream` (test sink).
- **Workflow** — the state machine. `update()` walks `_sleep → _confirmContact → _boot → _run`, driving `CarEvent` mode and `Hudisplay` page. `_run` owns page selection via the RPM+handbrake criterion below (the old `_isStationary` heuristic in `Hudisplay::requestAnimationFrame` is gone). RPM-absence is detected by a staleness counter (`RPM_ABSENT_TICKS`).
- **PowerManager** (`Energy`) — sleep / wake, CPU clock scaling, watchdog timing, wall-clock `realMillis()`. `_sleep` calls `Energy.shutdown()` (wakes on a FALLING INT = MCP2515 bus activity). Sleep/wake is board-specific and lives behind this layer: classic-AVR does real PWR_DOWN + WDT; RP2040 does `__wfi`/`__wfe` interrupt-driven sleep on the GP11 INT today, with XOSC-dormant (sub-mA) as a follow-up; ESP32-C3 deep-sleep and the megaavr/SAMD ports are still no-ops.

### Display-limit helpers

- `vw-hud/_hudmath.h` holds the pure numeric display-limit helpers (fuel/RPM/consumption pixel clamps, label offsets, average and distance zero-guards, µl rollover delta) — dependency-free, no U8g2 coupling.
- It is **code-generated**: `python3 tools/gen_hudmath.py` regenerates it from `// HUDMATH-BEGIN/END` marker blocks (idempotent; `--check` in CI). `test/test_hud.cpp` covers it with boundary tests.

### DBC abstraction

- No runtime DBC interpreter (KBs of flash, hundreds of bytes RAM, float math — non-viable on AVR, pointless for one car).
- The PQ decode is hardcoded behind the `CarEvent` interface: `switch(arbitrationID)` + mask/shift per signal (~hundreds of bytes flash, ~0 RAM, trivial CPU).
- The constants header can be code-generated from `vw_pq.dbc` via `cantools` (Python) in discodb2 — the DBC stays the single source of truth, at zero runtime cost.

## State machine

Designed for the **permanent-12V case** (OBD pin 16, possibly on a cut-timer, or a direct always-on 12V tap). The HUD is useless once the ignition is off. If power is actually cut at ignition-off, the board simply loses power and falls to `off` gracefully.

| State | Screen | Bus | Entry | Exit |
|---|---|---|---|---|
| off | off | off | power loss | power restored → `setup()` → boot |
| deep sleep | off | listening for wake INT | `Klemme_15`=0 confirmed | bus activity → ignition confirmation |
| ignition confirmation | off (key rule) | active, CPU minimal | woken by bus activity | `Klemme_15`=1 → boot; else (short delay) → deep sleep |
| boot | init → on | MODE_IDLE | ignition confirmed | fuel + odometer acquired (or timeout → degraded) → RPM>0 ? driving : idle |
| driving | PAGE_DRIVING | MODE_DRIVING | ignition AND (RPM>0 AND handbrake OFF) | → idle when (RPM=0 OR handbrake ON); `Klemme_15`=0 → deep sleep |
| idle | PAGE_IDLE | MODE_IDLE | ignition AND (RPM=0 OR handbrake ON) | → driving when (RPM>0 AND handbrake OFF); `Klemme_15`=0 → (trip summary briefly) → deep sleep |

### driving / idle criterion

- idle = ignition AND (**RPM=0 OR handbrake ON**).
- driving = ignition AND (**RPM>0 AND handbrake OFF**).
- These are strict De Morgan complements — no overlap, no flapping across the four cases.
- Engine running + handbrake pulled = idle (just parked → show trip summary). Red light (RPM>0, handbrake off) = driving.
- "RPM=0" manifests as the **absence of the RPM trame** in the window: the ECU stops broadcasting Motor_1 when the engine is off rather than sending a 0.
- Handbrake is coded as an **optional** condition so the machine degrades to RPM-only if the signal is unusable (see "To verify on bench").

### Wake strategy

- Chosen: "bus activity → check `Klemme_15` → re-sleep if no ignition". Stay ultra-economical and **screen OFF** until `Klemme_15` is confirmed.
- A dedicated wire on the ignition line (hardware INT, just-in-time wake) is kept only as a documented option for tinkerers, not the default.

## CAN signal map

From `opendbc/opendbc/dbc/vw_pq.dbc`. IDs in hex (decimal). Bit offsets are DBC convention. Where an info has several sources, all access paths are kept. **All entries to be confirmed on bench against this car.**

| Info | Message (ID) | Signal | Decode |
|---|---|---|---|
| RPM | Motor_1 (0x280 / 640) | `Motordrehzahl` | bit16, 16b, ×0.25 U/min |
| Speed | Kombi_1 (0x320 / 800) | `Geschwindigkeit__Kombi_1_` | bit25, 15b, ×0.01 km/h |
| Speed (displayed) | Kombi_1 (0x320 / 800) | `Angezeigte_Geschwindigkeit` | bit46, 10b, ×0.32 |
| Speed (wheel / ABS ref) | Bremse_1 (0x1A0 / 416) | `BR1_Rad_kmh` | wheel speed |
| Fuel level | Kombi_1 (0x320 / 800) | `Tankinhalt` | bit16, 7b, 0–126 litres (direct) |
| Fuel warning | Kombi_1 (0x320 / 800) | `Tankwarnung` | bit23 |
| Fuel stop | Kombi_1 (0x320 / 800) | `Tankstop` | bit15 |
| Odometer | Kombi_3 (0x520 / 1312) | `Kilometerstand` | bit40, 20b, km (direct) |
| Fuel consumption | Motor_5 (0x480 / 1152) | `MO5_Verbrauch` | bit16, 15b, cumulative µl counter |
| Fuel consumption (norm.) | Motor_Flexia | `Normierter_Verbrauch` | — |
| Gear (cluster display) | Getriebe_2 (0x540 / 1344) | `Ganganzeige_Kombi___Getriebe_Va` (bit56, 4b), `eingelegte_Fahrstufe` (bit60, 4b) | what the cluster shows |
| Gear (target) | Getriebe_1 (0x440 / 1088) | `GE1_Zielgang` | bit8, 4b, VAL table: 0=P/disengaged, 1–5=gears, 6=1m, 7=R, 8=6th, 9=7th, 10=8th, 14=undef, 15=fail |
| Gear (fallback) | — | — | ratio RPM/speed clustering |
| Ignition / contact | ZAS_1 (0x572 / 1394) | `Klemme_15__Z_ndung_ein_` (bit1), `Klemme_50__Starten_` (bit3), `Klemme_X__Startvorgang_` (bit2) | drives state transitions |
| Handbrake | Kombi_1 (0x320 / 800) | `Handbremserinnerung_s_Lampe` | bit1 — reminder lamp (manual handbrake) |
| Reverse | Getriebe_2 (0x540 / 1344) | `GK1_Rueckfahr` (bit28), `GK1_RueckfahrSch` (bit17) | also `Rueckfahrlicht` elsewhere |
| Seatbelt (nice-to-have) | Airbag_1 (0x050 / 80) | `Gurtschalter_Fahrer` (bit12), `Gurtschalter_Beifahrer` (bit14), `Gurtwarnung_Fahrer` (bit13), `Gurtwarnung_Beifahrer` (bit15), `Kindersitzerkennung` (bit10) | — |
| Seat occupancy (nice-to-have) | Airbag_2 (0x550 / 1360) | `Belegungserkennung_Beifahrersit` (bit8), hinten_links (bit9), rechts (bit10), mitte (bit11) | — |
| Seat position (nice-to-have) | Sitz_info (0x534 / 1332) | driver / passenger seat position | — |

The core driving page needs only **two MCP2515 filters**: 0x280 (Motor_1 / RPM) and 0x320 (Kombi_1 / speed + fuel).

Data-model consequences of the native-CAN decode:
- Fuel in **litres** direct (not %255) → impacts `_drawTank` / `_getTankValue`.
- Odometer in **km** direct (not the 4-byte hectometer hack).
- Fuel consumption via cumulative **µl counter** (better than the MAF estimate).

## Display model

### Frame and sliding window

- **frame** = one screen render cycle = **64 ms** (~15.6 Hz). Matches `requestAnimationFrame` / `_frameIndex` / `FRAME_DURATION_MS`.
- 64 ms is the watchdog quantum (`PowerManager::T_64MS`), so the frame is a clean multiple of the CPU timer.
- **Sliding window** = **64 frames = 4.096 s**. Both powers of 2: the window index wraps by mask `& 63` (no division), duration via `>> 12` (4096 ms, already `_millisIn4s`).
- The window is wall-clock (gated by `realMillis()`), so it is CPU-independent and identical on every board.
- RAM cost = 1 byte / sample / signal → 3 lists × 64 = **192 bytes**. Fallback to 32 frames (96 B) only if RAM forces it (choppier RPM gauge).

Two data notions:
- **temps réel** — instantaneous value.
- **fenêtre glissante** — keeps MIN and MAX over the window. RPM gauge shows a min↔max bar; speed shows windowed max; consumption bar shows min↔max.

### Cadence model — driven by the data's NATURE, not its importance

- A **littéraire** datum (digits to read) refreshed too often flickers and becomes unreadable (observed on speed: 107/108/107…). It needs a LOW cadence AND a redraw only when the *displayed* (quantized) value changes — the value is held/frozen between updates. This fixes flicker and saves render.
- A **graphique** datum (gauge / bar / needle) can move fast: the eye reads a shape, not digits → high cadence is fine.

Named driving zones (see [zones-driving.svg](zones-driving.svg)). Category letters: **R** RPM bar, **F** Fuel bar, **C** Consumption, **S** Speed, **G** Gear, **T** Trip.

- **R — RPM bar**: `R1` min edge, `R2` current cursor, `R3` max edge.
- **F — Fuel bar**: `F1` fuel cursor.
- **C — Consumption**: `C1` min edge, `C2` current cursor, `C3` max edge.
- **S — Speed**: `S1` current, `S2` max.
- **G — Gear** (each gear renders at its own on-screen position, per `_drawGearPosition`): `G1` gear 1 @ (123,32), `G2` gear 2 @ (105,32), `G3` gear 3 @ (18,22), `G4` gear 4 @ (0,22), `G5` gear 5 @ (0,32), `GN` neutral @ (18,32), `GR` reverse @ (18,32) — same cell as GN.
- **T — Trip**: `T1` distance travelled, `T2` elapsed time (h:mm).

Named idle zones (see [zones-idle.svg](zones-idle.svg)). A thin rule splits **vehicle identity** (above) from **this trip** (below). Category letters: **V** VIN, **O** Odometer, **T** Trip, **A** Averages, **F** Fuel bar. Layout is FIXED across boards — on light boards (e.g. 328) the VIN row stays blank (no reflow).

- **V — VIN** (above rule): `V1` VIN, positive text with a thin rule under it — board-gated (capable boards; read listen-only from `Ident` 0x5D2, multiplexed).
- **O — Odometer** (above rule): `O1` total odometer (km).
- **A — Averages** (below rule, middle row, with units): `A1` average speed km/h (left), `A2` average consumption l/100 (right).
- **T — Trip** (bottom line — **same position & style as the DRIVE trip line** so it's identical across screens; no units): `T1` distance (left), `T2` elapsed time h:mm (right).
- **F — Fuel bar**: `F1` fuel cursor (current level), `F2` start separator (level at trip start), `F3` fuel left in litres (carved), `F4` fuel burnt in litres (XOR).

SVG convention: a zone disabled in code (commented out) is drawn dimmed, with its legend in italics. Within a category, MIN comes first and MAX last; the current cursor sits between them (min ≤ current ≤ max).

Proposed driving-page zoning and cadences:

| Zone | Content | Nature | Cadence |
|---|---|---|---|
| Top band | RPM gauge + min↔max bar | graphique | 1/1 (64 ms) |
| Bottom | Fuel gauge + consumption min↔max bar | graphique | 1/4 (256 ms) |
| Side | Engaged gear | littéraire | 1/4 (~256 ms) |
| Center | Speed (big number) | littéraire | 1/8 (512 ms) + held while unchanged |
| Corner | Max speed | littéraire | held, recompute 1/16 (~1 s) |
| Line | Trip distance | littéraire | held, 1/16 |
| Line | Duration (to the minute) | littéraire | held, check 1/16 |
| Bottom (idle) | Fuel left (litres) | littéraire | held, slow |

Only RPM is 1/1; everything else round-robins across frames to spread render load.

### Vocabulary (locked — one word per concept, no synonyms)

| Term | Meaning |
|---|---|
| trame | raw CAN message (arbitration ID + bytes) — the atomic MCP2515 read |
| signal | a decoded value inside a trame (bit offset + length + scale per DBC); one trame carries several signaux |
| frame | one screen render cycle |
| cadence | a zone's refresh rate, as `1/N frames` |
| zone | a single screen region (the unit) |
| zoning | the partition of a state's screen into zones (the layout plan) — not a synonym of zone |
| graphique / littéraire | the nature of a displayed datum |

"frame" never refers to CAN.

## To verify on bench (discodb2)

- All CAN IDs, bit offsets and scales above — the DBC is a strong starting point, not a per-VIN guarantee.
- Bus bitrate per tapped harness (PQ powertrain 500 kbps; comfort/Kombi possibly 100 kbps).
- Cluster messages (fuel `Tankinhalt`, odometer `Kilometerstand`) may be **absent** on a powertrain-only tap. The odometer is rated only ~40% likely to appear on CAN. boot needs a timeout (degraded mode) if they never arrive.
- Whether `Handbremserinnerung_s_Lampe` reflects "handbrake engaged" (usable) or only a drive-off nag (unusable → fall back to RPM-only).
- Whether the OBD 12V is cut at ignition-off or kept on a timer (drives the power assumption — and whether a CANBed XOSC-dormant hang would self-clear at the next key).
- **CANBed RP2040 port** — its own structured acceptance plan (SPI re-route, INT polarity, crystal, sleep/wake) is tracked as hypotheses H1–H11 in [RECETTE-CANBED-RP2040.md](./RECETTE-CANBED-RP2040.md).

## Remaining work

The CAN pivot is done: `CarEventCan` filters by arbitration ID and decodes by bit offset, `Klemme_15` (ignition) is decoded and drives all transitions, `Energy.shutdown()` is wired (real PWR_DOWN), `_boot` has a tick timeout into degraded mode, and the `// TODO REMOVE` RPM/handbrake hacks are replaced by the locked state machine. What remains:

- **Bench verification** (discodb2, against this car): all CAN IDs / bit offsets / scales; deep-sleep wake on the MCP2515 INT0; the idle→driving filter reconfig (`setMode` + `switchOn`) for dropped frames; the tick thresholds (`CONFIRM` / `BOOT` / `RPM_ABSENT`).
- **Board ports**: a `PowerManager` port for the Nano Every (megaavr) and SAMD21; the RP2040 XOSC-dormant depth (sub-mA); ESP32-C3 deep-sleep + its `CarEventTwai` backend.
