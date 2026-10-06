#ifndef RfRemoteCc1101_HPP_
#define RfRemoteCc1101_HPP_

#include "RfRemote.h"
#include "RfCc1101.h"
#include "RfLibrary.h"
#if defined(VH_RF_PIO)
#include "RfOokPio.h"        // RP2040 PIO/DMA: non-blocking GDO0 (OLED stays live)
#endif

/**
 * Real emission backend for the rf-replay add-on: B2 replays the selected .sub
 * on the CC1101. Ties the pieces together —
 *   RfLibrary : names + frequency + lazy timings (LittleFS)
 *   RfCc1101  : OOK-async TX framing (SPI config, STX/SIDLE)
 *   GDO0 data : bit-bang (default, BLOCKING ~0.5-1 s) OR, with VH_RF_PIO on
 *               RP2040, a PIO+DMA waveform so core0 keeps rendering the OLED.
 * and inherits the B1/B2/timeout state machine from RfRemote.
 *
 * Needs VH_RF_CC1101 defined + the CC1101 wired. VH_RF_PIO additionally offloads
 * GDO0 to PIO (see RfOokPio.h). Register/timing values are bench-pending.
 */
class RfRemoteCc1101 : public RfRemote {

  public:
    void setRadio(RfCc1101* radio)     { this->_radio = radio; }
    void setLibrary(RfLibrary* library){ this->_lib = library; }

  protected:
    void _transmitStart(const uint8_t index) override
    {
        this->_sf.count = 0;
        if (this->_radio == nullptr || this->_lib == nullptr) { return; }

        this->_sf = this->_lib->loadTimings(index, this->_buf, RF_MAX_TIMINGS);
        if (this->_sf.count == 0 || !this->_sf.isRaw) { return; }  // nothing / not RAW

        unsigned long hz = this->_lib->freqHz(index);
        if (hz == 0UL) { hz = 433920000UL; }
        this->_radio->setFrequencyHz(hz);

        #if defined(VH_RF_PIO) && defined(ARDUINO_ARCH_RP2040)
        if (!this->_pio.ready()) { this->_pio.begin(VH_RF_GDO0); }  // one-time SM/DMA claim
        this->_radio->beginTx();                   // enter async TX
        this->_pio.send(this->_buf, this->_sf.count); // DMA-driven GDO0 (non-blocking)
        #else
        this->_radio->transmit(this->_buf, this->_sf.count);  // BLOCKING OOK bit-bang
        this->_radio->sleep();
        #endif
    }

    bool _transmitBusy(void) override
    {
        #if defined(VH_RF_PIO) && defined(ARDUINO_ARCH_RP2040)
        return (this->_sf.count != 0) && this->_pio.busy();
        #else
        return false;                               // blocking path already finished
        #endif
    }

    void _transmitEnd(void) override
    {
        #if defined(VH_RF_PIO) && defined(ARDUINO_ARCH_RP2040)
        this->_radio->endTx();                      // SIDLE + sleep after the waveform
        #endif
    }

    RfCc1101*       _radio = nullptr;
    RfLibrary*      _lib   = nullptr;
    SubFile         _sf;                            // last loaded header/count
    SubfileDuration _buf[RF_MAX_TIMINGS];           // one replay's timings (lazy-loaded)
    #if defined(VH_RF_PIO) && defined(ARDUINO_ARCH_RP2040)
    RfOokPio        _pio;
    #endif
};


#endif
