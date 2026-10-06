#ifndef RfCc1101_H_
#define RfCc1101_H_

#include <Arduino.h>
#include <SPI.h>
#include "_wiring.h"
#include "_subfile.h"

/**
 * Minimal CC1101 driver for OOK-async .sub REPLAY (TX only). Not a general
 * CC1101 library — just the register set needed to key the carrier from the raw
 * timing stream of a Flipper .sub (see doc/SUBGHZ-FORMAT.md).
 *
 * Path: configure over SPI (CS = VH_SPI_CS_RF) -> enter async TX (STX) -> drive
 * the data pin GDO0 (VH_RF_GDO0) HIGH/LOW with the .sub microsecond timings ->
 * SIDLE. During the bit-bang NO SPI happens, so the shared bus (MCP2515 + OLED)
 * stays free. The CC1101 is the same chip the Flipper uses, so what it captured
 * replays identically.
 *
 * !!! BENCH VALIDATION REQUIRED !!!  The register VALUES below (OOK, async,
 * GDO0-as-TX-data, PATABLE power) are datasheet/known-config derived but UNtested
 * on this module. The STRUCTURE (SPI config -> STX -> GDO0 bit-bang -> SIDLE) is
 * the contract; tune the bytes at the bench with a receiver. XTAL assumed 26 MHz.
 */
class RfCc1101 {

  public:
    void setPins(const uint8_t csPin, const uint8_t gdo0Pin)
    {
        this->_cs   = csPin;
        this->_gdo0 = gdo0Pin;
    }

    bool begin(void)
    {
        pinMode(this->_cs, OUTPUT);
        digitalWrite(this->_cs, HIGH);
        pinMode(this->_gdo0, OUTPUT);
        digitalWrite(this->_gdo0, LOW);
        SPI.begin();
        return this->_reset();
    }

    // Set the carrier frequency (Hz) from the .sub `Frequency` field.
    void setFrequencyHz(const unsigned long hz)
    {
        // FREQ = f * 2^16 / fXOSC   (fXOSC = 26 MHz)
        const uint32_t word = (uint32_t)(((uint64_t)hz << 16) / 26000000UL);
        this->_writeReg(REG_FREQ2, (uint8_t)(word >> 16));
        this->_writeReg(REG_FREQ1, (uint8_t)(word >> 8));
        this->_writeReg(REG_FREQ0, (uint8_t)(word));
    }

    // Load the OOK + async-serial-TX register profile (call once after begin()).
    void configOokAsyncTx(void)
    {
        this->_writeReg(REG_IOCFG0,   0x2E); // GDO0 = high-Z -> MCU drives it as async TX data
        this->_writeReg(REG_IOCFG2,   0x2E); // GDO2 unused (high-Z)
        this->_writeReg(REG_FIFOTHR,  0x47);
        this->_writeReg(REG_PKTCTRL0, 0x32); // async serial mode, no CRC, infinite length
        this->_writeReg(REG_FREND0,   0x11); // use PATABLE index 0/1 (OOK on/off)
        this->_writeReg(REG_MDMCFG2,  0x30); // MOD_FORMAT=011 (ASK/OOK), SYNC_MODE=000 (none)
        this->_writeReg(REG_MDMCFG1,  0x00);
        this->_writeReg(REG_MCSM0,    0x18); // auto-calibrate on IDLE->TX
        // PATABLE: index0 = carrier OFF, index1 = carrier ON (power). 0xC0 ~ +10 dBm @433.
        this->_writeReg(REG_PATABLE,  0x00);
        // second PATABLE byte (ON level) written via burst below
        this->_writePaOok(0xC0);
    }

    /**
     * Replay a parsed .sub: `t` = signed microseconds (+ carrier ON, - OFF),
     * `n` = sample count. BLOCKING (~0.5-1 s). Keeps SPI idle throughout.
     * (Advanced: offload GDO0 to RP2040 PIO / core1 to keep the OLED live — TODO.)
     */
    void transmit(const SubfileDuration* t, const size_t n)
    {
        this->_strobe(CMD_STX);               // enter TX (async)
        for (size_t i = 0; i < n; ++i) {
            const long d = (long)t[i];
            digitalWrite(this->_gdo0, (d > 0) ? HIGH : LOW);
            this->_delayUs((unsigned long)((d > 0) ? d : -d));
        }
        digitalWrite(this->_gdo0, LOW);
        this->_strobe(CMD_SIDLE);
    }

    // Non-blocking TX framing (used with the PIO engine, which drives GDO0):
    //   beginTx() -> enter async TX ; [PIO plays GDO0] ; endTx() -> idle + sleep.
    void beginTx(void) { this->_strobe(CMD_STX); }
    void endTx(void)   { this->_strobe(CMD_SIDLE); this->_strobe(CMD_SPWD); }

    void sleep(void) { this->_strobe(CMD_SPWD); }  // SPWD: power-down (~200 nA)

  protected:
    // --- register / strobe addresses (CC1101 datasheet) ---
    enum {
        REG_IOCFG2 = 0x00, REG_IOCFG0 = 0x02, REG_FIFOTHR = 0x03,
        REG_PKTCTRL0 = 0x08, REG_FREQ2 = 0x0D, REG_FREQ1 = 0x0E, REG_FREQ0 = 0x0F,
        REG_MDMCFG2 = 0x10, REG_MDMCFG1 = 0x11, REG_MCSM0 = 0x18, REG_FREND0 = 0x22,
        REG_PATABLE = 0x3E,
        CMD_SRES = 0x30, CMD_STX = 0x35, CMD_SIDLE = 0x36, CMD_SPWD = 0x39,
        WRITE_BURST = 0x40, READ_SINGLE = 0x80,
    };

    bool _reset(void)
    {
        digitalWrite(this->_cs, LOW);
        // wait for MISO low (chip ready) — bounded so a missing module can't hang
        for (uint16_t i = 0; i < 2000 && digitalRead(VH_SPI_CIPO); ++i) { delayMicroseconds(1); }
        SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
        SPI.transfer(CMD_SRES);
        SPI.endTransaction();
        digitalWrite(this->_cs, HIGH);
        delayMicroseconds(100);
        return true;   // (bench: verify PARTNUM/VERSION over SPI)
    }

    void _writeReg(const uint8_t addr, const uint8_t val)
    {
        SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
        digitalWrite(this->_cs, LOW);
        SPI.transfer(addr);
        SPI.transfer(val);
        digitalWrite(this->_cs, HIGH);
        SPI.endTransaction();
    }

    // PATABLE OOK: write [OFF=0x00, ON=on] as a burst so index1 = carrier power.
    void _writePaOok(const uint8_t on)
    {
        SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
        digitalWrite(this->_cs, LOW);
        SPI.transfer(REG_PATABLE | WRITE_BURST);
        SPI.transfer(0x00);
        SPI.transfer(on);
        digitalWrite(this->_cs, HIGH);
        SPI.endTransaction();
    }

    void _strobe(const uint8_t cmd)
    {
        SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
        digitalWrite(this->_cs, LOW);
        SPI.transfer(cmd);
        digitalWrite(this->_cs, HIGH);
        SPI.endTransaction();
    }

    // delayMicroseconds is only reliable up to ~16 ms on most cores; chunk it so
    // the long inter-frame gaps (and any clamped noise) keep correct timing.
    void _delayUs(unsigned long us)
    {
        while (us > 16000UL) { delayMicroseconds(16000); us -= 16000UL; }
        if (us) { delayMicroseconds((unsigned int)us); }
    }

    uint8_t _cs   = 0;
    uint8_t _gdo0 = 0;
};


#endif
