# Variant builds — required extra modules per board

The firmware compiles for six MCU architectures (see
[`../vw-hud/_wiring.h`](../vw-hud/_wiring.h)), but **each board family needs a
different CAN front-end and power feed**. Only the **CANBed RP2040** integrates
everything; every other build needs add-on modules. This document lists what to
add per build. Implementation stays on paper — pins live in `_wiring.h` (most
still marked bench-guess), wiring is theory until verified on the car.

> Reference assembly (CANBed, integrated) is detailed in
> [`README.md`](./README.md). This file covers the *variant* builds.

---

## Decision table

| Build (arch) | CAN front-end to add | Logic level | Power feed to add | Notes |
|---|---|---|---|---|
| **CANBed RP2040** (`ARDUINO_ARCH_RP2040`, the board) | **none** — MCP2515 + MCP2551 onboard | 3.3 V | **none** — 9–28 V reg onboard | reference build, see README |
| Uno / Nano classic (`ARDUINO_ARCH_AVR`) | external **MCP2515 module** | 5 V | 12 V → 5 V buck | baseline firmware target |
| Nano Every (`ARDUINO_ARCH_MEGAAVR`) | external **MCP2515 module** | 5 V | 12 V → 5 V buck | 4809 runs at 5 V |
| XIAO SAMD21 (`ARDUINO_ARCH_SAMD`) | external **MCP2515 module** | **3.3 V** | 12 V → 5 V buck → board LDO | ⚠️ level-shift / 3.3 V module (below) |
| bare Pi Pico RP2040 (`ARDUINO_ARCH_RP2040`, not CANBed) | external **MCP2515 module** | **3.3 V** | 12 V → 5 V buck → VSYS | ⚠️ level-shift / 3.3 V module (below) |
| XIAO ESP32-C3 (`ARDUINO_ARCH_ESP32`) | **CAN transceiver only** (TWAI controller is internal) | 3.3 V | 12 V → 5 V buck | not an MCP2515 module |

Common to **all** builds: the car tap (OBD-II), bus terminator OFF, and 12 V
transient protection — see *Car interface* below.

---

## A — MCP2515-module builds (Uno, Nano, Nano Every, XIAO SAMD21, bare Pico)

These MCUs have no CAN peripheral, so they need a complete **external MCP2515
module** = MCP2515 controller **+** an onboard transceiver (TJA1050 / MCP2551) **+**
its own crystal. The cheap "blue" module is the usual part.

**Wiring** (SPI + interrupt), pins per the arch block in `_wiring.h`:
`SCK`, `MOSI` (COPI), `MISO` (CIPO), `CS` = `VH_SPI_CS_CANBUS`, `INT` =
`VH_INT_CANBUS`, plus `VCC`/`GND`. The OLED shares the same SPI bus on its own
`CS`/`DC`/`RES`.

**Crystal must match the firmware.** `VH_CAN_CRYSTAL` in `_wiring.h` must equal
the crystal physically marked on the module:
- blue modules are usually **8 MHz** → `MCP_8MHZ`,
- some are **16 MHz** → `MCP_16MHZ` (same as the CANBed).
A mismatch makes the bit timing wrong → no frames decode. **Read the can marking,
do not assume.**

**⚠️ 3.3 V boards (SAMD21, bare Pico) — level-shift gotcha.** A standard blue
module powers its MCP2515 + TJA1050 at **5 V**, so its SPI lines are 5 V:
- `MISO` and `INT` drive **5 V into 3.3 V GPIO** → over-voltage, can damage the pin;
- `SCK`/`MOSI`/`CS` driven at 3.3 V may be marginal against the 5 V module's input
  threshold.

Two clean options:
1. a **3.3 V-native module** (MCP2515 powered at 3.3 V + an SN65HVD230 transceiver),
   wired directly, **or**
2. a **bidirectional level shifter** on `SCK`/`MOSI`/`CS`/`MISO`/`INT` between a
   5 V module and the 3.3 V MCU.

5 V boards (Uno, Nano, Nano Every) wire a 5 V module directly — no shifting.

**Power.** Add a **12 V → 5 V automotive buck** (load-dump / spike tolerant).
- 5 V boards: feed 5 V to the board's 5 V/VIN pin and to the module VCC.
- 3.3 V boards: feed 5 V to the board's 5 V/VBUS/VSYS pin; the onboard regulator
  makes 3.3 V. Power the module per option 1/2 above.

---

## B — ESP32-C3 build (native TWAI)

The ESP32-C3 has the **TWAI** CAN controller built in, so an MCP2515 module is
**not used**. What is missing is only the **physical layer**: add a **CAN
transceiver**, nothing else.

- **Transceiver**: **SN65HVD230** (3.3 V, pairs natively with the ESP32) or a
  3.3 V-I/O variant such as **TJA1051T/3**. Avoid 5 V-only transceivers — the
  ESP32-C3 GPIO are **not 5 V tolerant**.
- **Wiring**: transceiver `TXD`/`RXD` ↔ the TWAI `TX`/`RX` GPIO defined in the
  ESP32 block of `_wiring.h`; transceiver `CANH`/`CANL` to the bus; `VCC` = 3.3 V,
  `GND` common.
- **Firmware** drives TWAI in **listen-only** at **500 kbit/s** (the PQ powertrain
  bus) — the HUD never ACKs or transmits, so it cannot disturb the car.
- **Power**: 12 V → 5 V buck → the XIAO 5 V pin (onboard LDO makes 3.3 V).

> The TWAI controller is production-grade, but it is only the controller — without
> a transceiver the GPIO must never touch CANH/CANL.

---

## Car interface (all builds)

Tap the **OBD-II** port (under the dash) — least intrusive, gives power + bus in
one connector:

| OBD-II pin | Signal |
|---|---|
| 6 | CAN-H |
| 14 | CAN-L |
| 4 | chassis GND |
| 5 | signal GND |
| 16 | +12 V (permanent battery) |

- **Bus terminator OFF** on the module/CANBed — the bus is already terminated by
  its two end nodes; a third 120 Ω would overload it.
- **+12 V pin 16 is permanent** → either rely on firmware deep-sleep + low
  quiescent draw (measure it), or take a **switched** 12 V from an ignition fuse
  (add-a-fuse) so the HUD dies with the key.
- **Transient protection** on the 12 V feed: fuse + reverse-polarity diode + TVS;
  a common-mode choke on CANH/CANL is nice-to-have. The CANBed's 9–28 V regulator
  already absorbs most of this; the buck-based builds need it added explicitly.
- **⚠️ Verify the signals are actually on the OBD bus** with `discodb2` before
  committing: confirm `0x280` (RPM) and `0x320` (speed + fuel) reach OBD pins 6/14
  on this Sharan. The gateway may not bridge all Kombi/comfort traffic — if a
  signal is missing, tap behind the instrument cluster instead (more intrusive).

---

## Per-build shopping list (beyond MCU + OLED)

- **CANBed RP2040**: OBD-II cable only (+ thin 4-wire to the screw terminal).
- **Uno / Nano / Nano Every**: MCP2515 module (crystal-matched) · 12 V→5 V buck ·
  12 V protection · OBD-II cable.
- **XIAO SAMD21 / bare Pico**: 3.3 V MCP2515 module *or* 5 V module + level shifter ·
  12 V→5 V buck · 12 V protection · OBD-II cable.
- **XIAO ESP32-C3**: 3.3 V CAN transceiver (SN65HVD230 / TJA1051T/3) · 12 V→5 V
  buck · 12 V protection · OBD-II cable.
