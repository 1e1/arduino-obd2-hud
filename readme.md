# VW Head-Up Display

[![GitHub release](https://img.shields.io/github/v/release/1e1/arduino-obd2-hud?style=flat-square)](https://github.com/1e1/arduino-obd2-hud/releases)
[![GitHub commit activity](https://img.shields.io/github/commit-activity/m/1e1/arduino-obd2-hud?style=flat-square&color=brightgreen)](https://github.com/1e1/arduino-obd2-hud/commits/master)
[![License](https://img.shields.io/github/license/1e1/arduino-obd2-hud?style=flat-square)](https://github.com/1e1/arduino-obd2-hud/blob/master/LICENSE)

![lcd](./doc/lcd.jpg)

A self-built Head-Up Display for a VW Sharan (Sound, TSI 1.5, 2018, manual gearbox, platform PQ). It taps the car's **native CAN bus** — not OBD-II — and decodes raw frames (RPM, speed, fuel, gear, ignition) against `opendbc/vw_pq.dbc`, then renders a driving page and an idle page on a 128x64 OLED. Swappable backends (`CarEvent` source / `Hudisplay` renderer / `Workflow` state machine / `PowerManager`) keep it portable across boards and bench-testable.

- Live demo / simulator: [hud-simulator.html](https://1e1.github.io/arduino-obd2-hud/hud-simulator.html)
- Design doc: [doc/DESIGN.md](./doc/DESIGN.md)

## Wiring

![wiring](./doc/wiring.png)

The OLED and the MCP2515 share the SPI bus (SCK / MOSI / MISO); each has its own chip-select. Pins below are the classic Nano block from [`vw-hud/_wiring.h`](./vw-hud/_wiring.h).

| Signal | Pin |
|---|---|
| SCK | D13 |
| MOSI | D11 |
| MISO | D12 |
| MCP2515 CS | D9 |
| OLED CS | D8 |
| OLED DC | D5 |
| OLED RESET | D4 |
| MCP2515 INT | D2 |

### Optional RF-replay add-on

An optional module (enable with `-DVH_FEATURE_RF=1`, RP2040 "comfortable" tier) adds a **CC1101** OOK transmitter + **2 buttons** to replay garage-door `.sub` captures recorded on a Flipper Zero. Schematic: [`hardware/2d-models/rf-module-principle.svg`](./hardware/2d-models/rf-module-principle.svg) · operation: [`doc/SUBGHZ-FORMAT.md`](./doc/SUBGHZ-FORMAT.md).

| Signal | RP2040 pin | Macro |
|---|---|---|
| CC1101 CS | GP13 | `VH_SPI_CS_RF` |
| CC1101 GDO0 (OOK data) | GP12 | `VH_RF_GDO0` |
| CC1101 SCK / SI / SO | GP2 / GP3 / GP4 | (shared SPI0) |
| B1 — menu | GP6 | `VH_BTN_MENU` |
| B2 — exec | GP7 | `VH_BTN_EXEC` |

Buttons are **active-low to GND** (internal pull-up). The CC1101 is 3.3 V — on a 5 V board it needs level-shifting (see [`hardware/VARIANTS.md`](./hardware/VARIANTS.md)). Disabled by default → the baseline build is byte-identical.

## CAN exploration

Signal discovery / reverse-engineering is done in the companion project **[discodb2](https://github.com/1e1/discodb2)** — a read-only (listen-only) toolkit that streams and decodes the bus from a laptop or Pi via a USB CAN adapter, with a zero-hardware simulator.

## hardware

- Arduino Nano classic / Uno (ATmega328P @16 MHz, 32 KB ROM / 2 KB RAM) — the boards the firmware builds for today
- MCP2515 CAN board (SPI) — crystal frequency must be declared in code (8 or 16 MHz per module)
- SSD1309 128x64 OLED (SPI)
- DCC 12V→5V converter

Other target boards (Nano Every, XIAO SAMD21, XIAO ESP32-C3, RP2040 Pico, CANBed RP2040) need a `PowerManager` port and are listed in [doc/DESIGN.md](./doc/DESIGN.md).
