#ifndef wiring_H_
#define wiring_H_

// =============================================================================
// Board-config foundation (single source of per-board truth).
//
// Keyed on the Arduino architecture macros the cores define:
//   ARDUINO_ARCH_AVR      classic ATmega328 (Uno / Nano classic)  -- BASELINE
//   ARDUINO_ARCH_MEGAAVR  ATmega4809 (Nano Every)
//   ARDUINO_ARCH_SAMD     Cortex-M0+ (Seeed XIAO SAMD21)
//   ARDUINO_ARCH_RP2040   RP2040 (Pi Pico / CANBed RP2040, MCP2515 onboard)
//   ARDUINO_ARCH_ESP32    ESP32-C3 (native TWAI -- MCP2515 transport N/A)
//
// Each block centralises, per board:
//   - SPI/I2C/INT/CS pinout
//   - VH_SPI_CS_CANBUS         MCP2515 chip-select
//   - VH_CAN_CRYSTAL           MCP2515 crystal (MCP_8MHZ / MCP_16MHZ) -- MUST
//                              match the physical module or comms fail silently
//   - VH_DISPLAY_FULLBUFFER    0 = U8g2 page mode (constrained), 1 = full buffer
//   - VH_U8G2_BUFFER_MODE      the U8g2 class infix: _1_ / _2_ / _F_
//   - HUD_VIN                  defined only on roomy boards (SAMD/RP2040/ESP32)
//   - VH_DEEP_SLEEP            1 = PowerManager real deep-sleep available (AVR
//                              only today); 0 = portable no-op/timed fallback
//
// MCP2515 crystal note (design-constraints): cheap blue modules are usually
// 8 MHz, sometimes 16; the CANBed RP2040 onboard controller is 16 MHz. There is
// NO auto-detection -- the value below is authoritative.
// =============================================================================

// Select the classic-AVR pinout for ARDUINO_ARCH_AVR (Uno / Nano classic).
#if defined(ARDUINO_ARCH_AVR)
#define __USE_NANO__
#endif


// -----------------------------------------------------------------------------
// ATmega328 -- Uno / Nano classic (BASELINE: must stay byte-identical)
// -----------------------------------------------------------------------------
#ifdef __USE_NANO__
#define VH_SCK                      13
#define VH_SPI_COPI                 11  // MOSI
#define VH_SPI_CIPO                 12  // MISO
#define VH_I2C_SCL                  19
#define VH_I2C_SDA                  18
#define VH_SPI_CS_CANBUS            9
#define VH_SPI_CS_SCREEN            8
#define VH_DC0_SCREEN               5   // DC
#define VH_RESET_SCREEN             4
#define VH_INT_CANBUS               2
#define VH_CAN_CRYSTAL              MCP_8MHZ   // MCP2515 module crystal — verify the physical marking (8.000 / 16.000)

#define VH_SERIAL_PORT              Serial
#define VH_SERIAL_SPEED             9600
#define VH_DISPLAY_FULLBUFFER       0
// page mode, 2-row U8g2 buffer (suits the 2 KB RAM ceiling)
#define VH_U8G2_BUFFER_MODE         _2_
#define VH_DEEP_SLEEP               1     // real AVR PWR_DOWN + WDT
// HUD_VIN intentionally UNDEFINED (no RAM headroom on 328)
#endif


// -----------------------------------------------------------------------------
// ATmega4809 -- Arduino Nano Every (arduino:megaavr)
// 48 KB flash / 6 KB RAM. Still "constrained" tier per design-constraints
// (page mode), but the megaavr has no classic-AVR WDT regs -> no real deep-sleep
// yet (PowerManager fallback). TODO: port to the megaavr RTC/WDT peripheral.
// -----------------------------------------------------------------------------
#if defined(ARDUINO_ARCH_MEGAAVR) && !defined(VH_SPI_CS_CANBUS)
#define VH_SCK                      13
#define VH_SPI_COPI                 11  // MOSI
#define VH_SPI_CIPO                 12  // MISO
#define VH_I2C_SCL                  19
#define VH_I2C_SDA                  18
#define VH_SPI_CS_CANBUS            9
#define VH_SPI_CS_SCREEN            8
#define VH_DC0_SCREEN               5   // DC
#define VH_RESET_SCREEN             4
#define VH_INT_CANBUS               2
#define VH_CAN_CRYSTAL              MCP_8MHZ   // TODO confirm module crystal

#define VH_SERIAL_PORT              Serial
#define VH_SERIAL_SPEED             9600
#define VH_DISPLAY_FULLBUFFER       0
// page mode (keep RAM low even at 6 KB)
#define VH_U8G2_BUFFER_MODE         _2_
#define VH_DEEP_SLEEP               0     // TODO: megaavr RTC/WDT sleep port
// HUD_VIN intentionally UNDEFINED (constrained tier)
#endif


// -----------------------------------------------------------------------------
// SAMD21 -- Seeed XIAO SAMD21 (CI FQBN Seeeduino:samd:seeed_XIAO_m0).
// 256 KB / 32 KB -> "comfortable": full buffer + partial updates, HUD_VIN.
// External MCP2515. XIAO SAMD21 exposes D0-D10; HW SPI is fixed on D8(SCK)/
// D9(MISO)/D10(MOSI), I2C on D4(SDA)/D5(SCL). The OLED uses HW SPI, so CS/DC/
// RST/INT take the remaining free pins (D0..D3, D6, D7) -- no overlap with the
// SPI data lines. All CS/DC/RST/INT assignments are BENCH-GUESS, verify wiring.
// -----------------------------------------------------------------------------
#if defined(ARDUINO_ARCH_SAMD) && !defined(VH_SPI_CS_CANBUS)
#define VH_SCK                      8   // XIAO SAMD21 SCK (fixed)
#define VH_SPI_COPI                 10  // MOSI (fixed)
#define VH_SPI_CIPO                 9   // MISO (fixed)
#define VH_I2C_SCL                  5   // (unused: OLED is on HW SPI)
#define VH_I2C_SDA                  4   // (unused)
#define VH_SPI_CS_CANBUS            7   // free pin (verify)
#define VH_SPI_CS_SCREEN            6   // free pin (verify)
#define VH_DC0_SCREEN               3   // DC (verify)
#define VH_RESET_SCREEN             2   // (verify)
#define VH_INT_CANBUS               1   // MCP2515 INT (verify)
#define VH_CAN_CRYSTAL              MCP_16MHZ  // TODO confirm module crystal

#define VH_SERIAL_PORT              Serial
#define VH_SERIAL_SPEED             115200
#define VH_DISPLAY_FULLBUFFER       1
// full frame buffer (1 KB, fine at 32 KB)
#define VH_U8G2_BUFFER_MODE         _F_
#define VH_DEEP_SLEEP               0     // TODO: SAMD standby/sleep (SYSCTRL + RTC wake)
#define HUD_VIN                           // roomy: render the VIN on the idle page
#endif


// -----------------------------------------------------------------------------
// RP2040 -- Raspberry Pi Pico / CANBed RP2040 (Earle Philhower core).
// 2 MB / 264 KB -> "comfortable". CANBed wires the MCP2515 ONBOARD to SPI0
// (SCK GP2 / MOSI GP3 / MISO GP4), CS GP9, INT GP11, 16 MHz crystal -- confirmed
// against the official Longan + Zephyr board pinout. A bare Pico uses an external
// module; only the wiring differs, the mcp_can SPI transport is identical.
// NOTE: the onboard MCP is NOT on the Philhower default SPI0 pins (18/19/16), so
// the SPI bus must be re-routed with SPI.setSCK(2)/setTX(3)/setRX(4) before
// SPI.begin() -- VH_SCK/COPI/CIPO below are currently consumed only by the
// (commented-out) SW-SPI display path. PRIORITISED target (enclosure).
// -----------------------------------------------------------------------------
#if defined(ARDUINO_ARCH_RP2040) && !defined(VH_SPI_CS_CANBUS)
#define VH_SCK                      2   // CANBed onboard MCP2515 on SPI0 SCK (GP2)
#define VH_SPI_COPI                 3   // MOSI (GP3)
#define VH_SPI_CIPO                 4   // MISO (GP4)
#define VH_I2C_SCL                  21
#define VH_I2C_SDA                  20
#define VH_SPI_CS_CANBUS            9   // CANBed RP2040 onboard MCP2515 CS (GP9)
#define VH_SPI_CS_SCREEN            17
#define VH_DC0_SCREEN               14  // DC
#define VH_RESET_SCREEN             15
#define VH_INT_CANBUS               11  // CANBed onboard MCP2515 INT (GP11, confirmed) [H3]
// [H6] Quartz du MCP2515 onboard = 16 MHz. Le marquage du composant est illisible
// a l'oeil sur la carte ; valeur confirmee par le devicetree Zephyr canbed_rp2040
// (`osc-freq = <16000000>`). Un quartz mal declare -> SILENCE TOTAL du CAN (aucune
// trame), pas d'erreur explicite. Cf. recette R1.
#define VH_CAN_CRYSTAL              MCP_16MHZ  // CANBed onboard crystal = 16 MHz (confirmed) [H6]

#define VH_SERIAL_PORT              Serial
#define VH_SERIAL_SPEED             115200
#define VH_DISPLAY_FULLBUFFER       1
// full frame buffer (ample RAM)
#define VH_U8G2_BUFFER_MODE         _F_
// VH_DEEP_SLEEP reste 0 = chemin PowerManager portable (pas de regs classic-AVR).
// MAIS ce chemin n'est PLUS un no-op sur RP2040 : shutdown() dort en __wfi et se
// reveille sur l'INT du MCP (GP11), standby() idle en __wfe. Seul le vrai dormant
// XOSC (sub-mA) reste a faire (phase 2, recette R6 / TODO PowerManager.cpp [H10]).
#define VH_DEEP_SLEEP               0
// [H10] Selecteur shutdown() RP2040 (resultat de la recette R6) :
//   0 = __wfi pilote par l'INT  -> SUR sur toute alim, ne peut pas figer (defaut).
//   1 = vrai XOSC-dormant sub-mA -> N'ACTIVER qu'apres validation banc ET si le
//       CANBed est sur 12V COMMUTE (un hang se debloque alors a la cle suivante).
#define VH_RP2040_DORMANT           0
#define HUD_VIN                           // roomy: render the VIN on the idle page
#endif


// -----------------------------------------------------------------------------
// ESP32-C3 -- Seeed XIAO ESP32-C3 (esp32:esp32). NATIVE TWAI controller, NO
// MCP2515 -> uses the dedicated CarEventTwaiVwPq transport (driver/twai.h) +
// an external CAN transceiver only (e.g. SN65HVD230 / TJA1051). VH_INPUT_TWAI
// tells the .ino to instantiate the TWAI backend instead of the MCP2515 one.
//
// The SPI pins below are kept only so the board-config stays uniform (the OLED
// is still SPI); VH_SPI_CS_CANBUS is intentionally LEFT UNDEFINED so the MCP2515
// transport is not selected. VH_TWAI_TX/RX feed twai_general_config_t.
// -----------------------------------------------------------------------------
#if defined(ARDUINO_ARCH_ESP32) && !defined(VH_INPUT_TWAI)
#define VH_INPUT_TWAI                     // select the native-TWAI backend in vw-hud.ino

// --- TWAI (native CAN) controller pins -- BENCH-GUESS, verify against transceiver
#define VH_TWAI_TX                  4   // XIAO ESP32-C3 D2 / GPIO4 -> transceiver TXD  (verify)
#define VH_TWAI_RX                  5   // XIAO ESP32-C3 D3 / GPIO5 -> transceiver RXD  (verify)

// --- OLED on SPI (CANBUS CS deliberately undefined: no MCP2515 on this board) ---
#define VH_SCK                      8
#define VH_SPI_COPI                 10  // MOSI
#define VH_SPI_CIPO                 9   // MISO
#define VH_I2C_SCL                  7
#define VH_I2C_SDA                  6
#define VH_SPI_CS_SCREEN            20  // verify on bench (XIAO ESP32-C3 header)
#define VH_DC0_SCREEN               21  // DC (verify)
#define VH_RESET_SCREEN             3   // (verify)

#define VH_SERIAL_PORT              Serial
#define VH_SERIAL_SPEED             115200
#define VH_DISPLAY_FULLBUFFER       1
#define VH_U8G2_BUFFER_MODE         _F_
#define VH_DEEP_SLEEP               0     // TODO: ESP32 esp_deep_sleep_start + GPIO/TWAI wake
#define HUD_VIN
#endif


// -----------------------------------------------------------------------------
// Cross-board defaults / safety net for an unrecognised arch.
// -----------------------------------------------------------------------------
#ifndef VH_DEEP_SLEEP
#define VH_DEEP_SLEEP               0
#endif
#ifndef VH_RP2040_DORMANT
#define VH_RP2040_DORMANT           0
#endif
#ifndef VH_U8G2_BUFFER_MODE
#define VH_U8G2_BUFFER_MODE         _2_
#endif
#ifndef VH_DISPLAY_FULLBUFFER
#define VH_DISPLAY_FULLBUFFER       0
#endif


// CAN bus bitrate — PQ powertrain is 500 kbps; the comfort/Kombi bus may be 100 kbps.
// Verify per the tapped harness (discodb2). Bus-level (not board-level) so shared.
#define VH_CAN_BITRATE              CAN_500KBPS

// MCP2515 INT-driven deep-sleep wake. Only meaningful where an INT pin exists
// (MCP2515 boards); the ESP32/TWAI board has no such pin -> PowerManager falls
// back to NOT_AN_INTERRUPT (its own default).
#ifdef VH_INT_CANBUS
#define LOWPOWER_INTERRUPT_PIN      digitalPinToInterrupt(VH_INT_CANBUS)
#endif


// Compose the concrete U8g2 display class from the per-board buffer-mode infix.
// 328 keeps _2_ (byte-identical baseline); roomy boards use _F_. Two-level
// macro indirection so VH_U8G2_BUFFER_MODE expands before pasting.
#define VH_U8G2_PASTE3(a, b, c)     a##b##c
#define VH_U8G2_JOIN(a, b, c)       VH_U8G2_PASTE3(a, b, c)
#define VH_U8G2_DISPLAY_CLASS \
  VH_U8G2_JOIN(U8G2_SSD1309_128X64_NONAME0, VH_U8G2_BUFFER_MODE, 4W_HW_SPI)


#endif
