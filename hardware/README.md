# Hardware — Main Assembly

Component catalogue for the **main HUD assembly** and home for its 3D-printable
enclosure models ([`3d-models/`](./3d-models/)).

The main assembly is intentionally a **two-board build**:

1. **SSD1309 2.42" 128×64 OLED** (the diymore module) — the display.
2. **CANBed RP2040 (Longan Labs)** — the controller **with the MCP2515 CAN
   controller integrated onboard** (plus an MCP2551 transceiver), so no separate
   CAN module is needed.

> The firmware also builds for a classic **Nano / Uno + external MCP2515 module**
> (the current CI target — see [`../readme.md`](../readme.md) and
> [`../doc/DESIGN.md`](../doc/DESIGN.md)). This folder documents the *integrated*
> CANBed assembly; the enclosure here is designed around it. The **extra modules
> needed for every non-CANBed build** (MCP2515 module, ESP32 transceiver, power,
> level-shifting) are catalogued in [`VARIANTS.md`](./VARIANTS.md).

## ⚠️ Reliability of the values below

| Category | Status | Note |
|---|---|---|
| Electrical (datasheet) | ✅ Confirmed | From vendor / IC datasheets. |
| Pinout / wiring | ✅ Confirmed | CANBed CAN pins are fixed onboard; OLED pins from the module. |
| **Mechanical dimensions** | ⚠️ **TO MEASURE** | **Do not 3D-model against these without calipers.** Vendor pages omit them and they vary per batch. |
| Power consumption | ⚠️ Estimate | Datasheet typicals; real draw depends on lit-pixel %, MCP TX activity, CPU clock. Measure in-car. |

Every value marked `⟨measure⟩` must be filled from the physical part with a
caliper before any enclosure is printed.

---

## Bill of materials (main assembly)

| # | Component | Role | Interface | Voltage |
|---|---|---|---|---|
| 1 | diymore 2.42" OLED, SSD1309 | Display (128×64) | SPI (4-wire) | 3.3 V |
| 2 | CANBed RP2040 (Longan 1030018) | MCU + CAN (MCP2515 + MCP2551) | USB / CAN DB9+terminal | 9–28 V in → 3.3 V/1 A out |
| — | Wiring harness | OLED ↔ CANBed SPI | 7 wires | — |
| — | 12 V car tap | Power feed to CANBed VIN | — | 12 V nominal |

The CANBed's onboard 9–28 V → 3.3 V/1 A regulator means **the assembly can be
fed directly from a 12 V car tap** — no separate DCC converter is required for
this build (unlike the Nano/Uno build, which needs a 12 V→5 V converter).

---

## Component 1 — diymore 2.42" OLED (SSD1309)

Product: <https://www.diymore.cc/products/2-42-inch-12864-oled-display-module-iic-i2c-spi-serial-for-arduino-c51-stm32-green-white-blue-yellow>
Dimensioned via the equivalent clone: <https://www.hicenda.com/product/242-inch-oled-module-05.html>

### Confirmed (full mechanical drawing — hicenda)
| Spec | Value |
|---|---|
| Driver IC | SSD1309 |
| Resolution | 128 × 64 px, monochrome (passive matrix, 1/32 duty) |
| Diagonal | 2.42" |
| **Module outline** | **72.00 ±0.5 × 43.00 ±0.5 × 5.90 (max) mm** |
| **Mounting holes** | **4 × Ø2.80 mm**, inset **2.10 mm** from every edge → pitch **67.80 (X) × 38.80 (Y) mm** |
| **Viewing area (glass window)** | **57.01 × 29.5 mm** — offset **7.48 mm** from left edge (≈ centred horizontally) |
| **Active area (pixels)** | **55.01 × 27.49 mm** |
| Thickness profile | 5.90 max overall (PCB ≈ 1.0 mm, front glass/display stack steps to 2.90 / 5.90 per drawing) |
| Pixel pitch / size | 0.39 × 0.39 mm / 0.43 × 0.43 mm |
| 7-pin header | 2.50 mm pitch, along the bottom edge (pins 1→7) |
| Supply voltage Vdd | 3.0 / **3.3** / 3.6 V (min/typ/max) |
| Logic current Idd | typ **180 µA**, max 300 µA (Ta=25 °C) |
| Display current ICC | **62 mA** typ @ 50 % pixels on (max 70); **113 mA** typ @ 100 % on (max 120) |
| Sleep current | typ 3 µA, max 15 µA |
| Operating / storage temp | −40 … +85 °C / −40 … +90 °C |
| Interface | SPI **or** I²C — selectable via resistors **R3 / R4 / R5** on the back |
| SPI pins (7) | `GND` `VCC` `SCL`(=SCK) `SDA`(=MOSI) `RES` `DC` `CS` |
| Backlight | None (self-emissive OLED) |
| Firmware driver | U8g2 `U8G2_SSD1309_128X64_NONAME0_2_4W_HW_SPI` (see [`../vw-hud/vw-hud.ino`](../vw-hud/vw-hud.ino)) |

✅ The OLED is now **fully dimensioned** from the manufacturer drawing — outline,
hole positions/Ø, viewing/active areas, and electrical figures are all confirmed.
The HUD draws mostly-dark pages, so real display current sits well below the
113 mA full-white figure — but **plan the enclosure window around the 57.01 × 29.5
viewing area**, not the 55.01 × 27.49 pixel area.

> Remaining caliper check (nice-to-have, not blocking): the V.A **vertical** offset
> from the top edge (drawing chain ≈ 5.11 mm) and the exact PCB-only thickness.

---

## Component 2 — CANBed RP2040 (Longan Labs 1030018)

Docs: <https://docs.longan-labs.cc/1030018/> · Product: <https://www.longan-labs.cc/1030018.html>

### Confirmed
| Spec | Value |
|---|---|
| MCU | RP2040, dual ARM Cortex-M0+, up to 133 MHz |
| Flash | 2 MB |
| RAM | 264 KB |
| **Board outline** | **56 × 41 mm** (V1.1) — corroborated by the ruler photo |
| **PCB thickness** | **1.0 mm** |
| **Edge layout** | RIGHT = CAN (screw terminal CANL/CANH/GND/VIN + DB9) · BOTTOM = micro-USB (left-of-centre) + SPI/SWD/debug headers · LEFT = 2×9 GPIO header · TOP = reset + I²C + UART + 120 Ω jumpers |
| CAN controller | **MCP2515 (onboard)** |
| CAN transceiver | **MCP2551 (onboard)** |
| CAN protocol | CAN 2.0B, standard 11-bit + extended 29-bit, up to 1 Mb/s |
| MCP2515 crystal | **16 MHz** → firmware must use `MCP_16MHZ` (≠ the 8 MHz blue modules; see [`../doc/DESIGN.md`](../doc/DESIGN.md)) |
| Power input | 9–28 V (suits a direct 12 V car tap) |
| Regulated output | 3.3 V / 1 A |
| USB | Micro-USB (programming + power) |
| CAN connectors | DB9 sub-D (OBD-II / CANopen selectable) **and** 4-pin terminal — both on one edge |
| Other connectors | 2× Grove HY2.0 (1× I²C `Wire1` SDA=6/SCL=7, 1× UART `Serial1`), 9×2 GPIO + 3×3 2.54 mm headers |
| Onboard features | CAN RX/TX LEDs, 120 Ω terminator switch, reset button |

### CAN / SPI wiring (onboard — fixed)
The MCP2515 is wired to the RP2040 **on the board**; you do not wire it yourself.
The existing `CarEventCan` (mcp_can lib) works as-is on CANBed — **`SPI_CS_PIN = 9`**
(confirmed in the Longan docs).

GPIO header map (9×2, from the datasheet pinout):

```
left  column:  D4  D5  D6  A3  D12  A0  A1  A2  5V
right column:  D0/RX D1/TX D2/SDA D3/SCL D8  D9  D10 D11 GND
```

`D9` = MCP2515 CS (used by the onboard CAN). I²C is `Wire1` (`SDA=6/SCL=7`, the
Grove I²C connector). ⟨verify⟩ the MCP2515 `INT` GPIO from the Longan example,
then add a `__USE_CANBED__` block in [`../vw-hud/_wiring.h`](../vw-hud/_wiring.h)
(today only `__USE_NANO__` and `__USE_XIAO__` blocks exist).

**Broken-out SPI header** (bottom edge, 2×3, for the OLED): silk reads
`IO8 SCK ISO` / `GND SI 3V` → `SCK`, `ISO` (=CIPO/MISO, unused for a write-only
display), `SI` (=COPI/MOSI), `IO8` (a free GPIO usable as a CS), plus `3V`/`GND`.

### To measure ⟨measure⟩
| Spec | Value | How |
|---|---|---|
| Total height with DB9 | ⟨measure⟩ (DB9 ~12.5, screw terminal ~10, µUSB ~3 — parts not mounted in the photo) | caliper |
| Mounting holes | observed **4 holes** (2 small on the LEFT corners, 2 larger on the RIGHT by the CAN block — likely M3) — Ø ⟨measure⟩ / positions ⟨measure⟩ | caliper |
| Connector edge offsets | ⟨measure⟩ — exact offset of µUSB / terminal / DB9 along their edges | enclosure cutouts |
| Idle / active current at 12 V in | ⟨measure⟩ | measure on the 12 V feed |

---

## OLED ↔ CANBed wiring (to define)

The OLED is write-only SPI; route it to the CANBed's broken-out **SPI header**
(bottom edge) for clock+data, and pick free GPIO for `DC`/`RES`/`CS`. **Do not use
`GP9`** (taken by the onboard MCP2515 CS). Proposed mapping (confirm pin labels on
the board + arduino-pico SPI instance before wiring):

| OLED pin | CANBed pin | Note |
|---|---|---|
| VCC | `3V` (SPI header) | 3.3 V |
| GND | `GND` (SPI header) | |
| SCL (SCK) | `SCK` (SPI header) | SPI clock |
| SDA (MOSI) | `SI` (SPI header) | = COPI/MOSI |
| CS | `IO8` (SPI header) ⟨confirm⟩ | free CS pin broken out next to SPI |
| DC | free GPIO ⟨define⟩ (e.g. a left-header pin, **not GP9**) | |
| RES | free GPIO ⟨define⟩ | |
| — | `ISO` (MISO) | unused — display is write-only |

> The Nano build's reference pinout (D13/D11/D9/D8/D5/D4/D2) is in
> [`../readme.md`](../readme.md) — useful as a layout model, not directly reusable
> (RP2040 GPIO numbering differs).

---

## Car connection & mounting (proposed consigne)

Diagrams: [`2d-models/connection-principle.svg`](./2d-models/connection-principle.svg)
· [`2d-models/mounting-principle.svg`](./2d-models/mounting-principle.svg)

**CAN connection — via OBD-II, no DB9 (slim cable at the HUD):**
- The DB9 plug is too bulky at the HUD end → **do not mount the DB9**. Wire the
  **4-pin screw terminal** (`CANL/CANH/GND/VIN`) — or solder directly to the pads
  for minimum height — so only a **thin 4-conductor cable** leaves the enclosure.
- Car end: a standard **OBD-II male plug** (bulk is fine there, hidden under the
  dash); prefer a low-profile/right-angle one, ideally a **pass-through** so the
  diagnostic port stays usable. OBD-II gives CAN-H (6), CAN-L (14), GND (4/5), and
  **+12V permanent (16)** → no separate power tap.
- **120 Ω terminator: OFF** — the bus is already terminated by its two end nodes;
  a third 120 Ω would load it.
- **+12V pin 16 is permanent** → relies on firmware sleep + low quiescent current
  (measure it). Battery-safe alternative: a **switched** 12V via an add-a-fuse on
  an ignition fuse.
- ⚠️ **Verify with discodb2** that both `0x280` (RPM) and `0x320` (speed+fuel) are
  broadcast on the bus reachable at OBD 6/14 on this Sharan — the gateway may not
  bridge all Kombi/comfort traffic. If a signal is missing → tap behind the cluster
  (more intrusive).

**Mounting — least intrusive, stable (braking & bumps):**
- A printed bracket that **hooks over the instrument-cluster cowl lip** (rigid
  existing edge) takes the **fore-aft braking load**; a **TPU/foam anti-slip foot**
  on the dash takes the vertical load and **damps vibration**. No adhesive, no
  drilling, reversible. No windshield suction.
- **Low CG** (CANBed mass at the bottom), **2-part** design (cradle stays, HUD
  clips out), cable **strain-relieved** and routed down to the OBD port.
- The bracket also sets the **panel tilt + hood** that serve the anti-reflection /
  anti-sun goals below.

---

## Thermal, optical & environment — HUD under the windshield

This is a **dashboard HUD sitting under the windshield**, so the dominant thermal
load is **not** the electronics — it is **solar gain**. A parked car's dash can
reach **70–90 °C** in summer sun, on top of the self-heating below.

**Component thermal limits (the binding constraints):**
- OLED operating max **+85 °C**, storage **+90 °C** → the tightest limit. Sustained
  high temp also accelerates OLED burn-in/aging.
- RP2040 / MCP2515 / MCP2551: industrial parts, typ. **+85 °C** ambient.
- The CANBed 9–28 V → 3.3 V regulator dissipates the 12 V→3.3 V drop; at OLED full
  draw (~0.12 A) that is modest, but it is a local hot spot.

**Self-heat budget (small):** OLED ≤ ~0.4 W (much less on dark HUD pages) + RP2040
+ regulator losses ≈ **~1 W order of magnitude**. Solar gain dwarfs it.

**Enclosure design implications (drive the heights, not component clearance):**
- **Ventilation over sealing**: passive convection — vents low + high (chimney
  effect), or an open-bottom shroud. Avoid a sealed black box on the dash.
- **Don't trap the OLED against hot plastic**: stand it off, leave an air gap
  behind the panel; keep the regulator/ICs in the ventilated zone.
- **Material/colour**: light colour + UV/heat-stable filament. **PLA softens
  ~50–60 °C → unsuitable on a sun-baked dash**; prefer **PETG / ASA / ABS / PC**.
- **Heights are free to grow for airflow** — there is no tight vertical packaging
  constraint, so favour standoffs and a vented cavity over a flat slim box.
- Consider shading/anti-glare hood geometry (it doubles as sun shielding for the
  panel face).

**Optical — avoid windshield reflections (a HUD-specific constraint):**
A bright display facing up toward the raked windshield throws a **ghost reflection**
into the driver's view. Mitigations to bake into the enclosure:
- **Tilt the panel** so its specular reflection is directed away from the eye line
  (the glass face should not be parallel to the windshield).
- **Matte / dark, non-glossy interior and bezel** around the panel — no shiny
  surfaces near the glass; matte black filament or a flat coat there.
- **A hood / visor** over the top of the panel (the same anti-sun shade) cuts the
  light path that creates the reflection.
- Keep the **bezel opening tight to the V.A (57.01 × 29.5)** so no lit border or
  PCB silk is visible/reflected.
- Favour the OLED's own contrast: dark HUD pages (few lit pixels) inherently
  reflect less — already the case for this firmware.

> Net: heights and vents in the 3D model are governed by **heat evacuation** (OLED
> **+85 °C** ceiling), and the **panel tilt + hood + matte bezel** are governed by
> **reflection control** — not by component height alone.

---

## 3D models — [`3d-models/`](./3d-models/)

Enclosure source will live there as parametric **OpenSCAD** (`.scad`) plus
exported `.stl`/`.3mf`. The model is parameterised so the `⟨measure⟩` values
above feed straight into variables.

**Before printing, fill every `⟨measure⟩` field above.** The print-critical set:

- [ ] OLED PCB outline + 4 hole positions + hole Ø
- [ ] OLED active-glass window size + offset from PCB edges
- [ ] OLED back-side tallest component height (standoff depth)
- [ ] CANBed board outline + mounting holes
- [ ] CANBed connector edges (USB, DB9, terminal) for cutouts
- [ ] Total stack height (OLED front glass → CANBed tall components)

---

## Sources

- diymore 2.42" OLED product page (driver, voltage, interface): <https://www.diymore.cc/products/2-42-inch-12864-oled-display-module-iic-i2c-spi-serial-for-arduino-c51-stm32-green-white-blue-yellow>
- [Waveshare 2.42" OLED wiki](https://www.waveshare.com/wiki/2.42inch_OLED_Module) — dimension reference for a *similar* module (≈70.9×43.4 mm)
- [CANBed RP2040 — Longan docs](https://docs.longan-labs.cc/1030018/)
- [CANBed RP2040 — product page](https://www.longan-labs.cc/1030018.html)
- Repo wiring / board notes: [`../readme.md`](../readme.md), [`../doc/DESIGN.md`](../doc/DESIGN.md), [`../vw-hud/_wiring.h`](../vw-hud/_wiring.h)
