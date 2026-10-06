#ifndef RfOokPio_H_
#define RfOokPio_H_

#include <Arduino.h>
#include "_subfile.h"

/**
 * RP2040 PIO + DMA engine that plays an OOK waveform on GDO0 WITHOUT the CPU —
 * so the OLED keeps refreshing (and CAN keeps being read) during a .sub replay,
 * instead of the ~0.5-1 s freeze of the blocking bit-bang (see the power-budget
 * analysis). A tiny PIO program holds the pin at a level for a DMA-fed duration;
 * core0 is free the whole time.
 *
 * GATED by VH_RF_PIO and RP2040-only. The CC1101 is still configured over SPI
 * (OOK async, STX/SIDLE) by RfCc1101; this engine only drives the GDO0 line.
 *
 * !!! BENCH VALIDATION REQUIRED !!! The PIO program is hand-assembled (3 words)
 * and the per-sample loop overhead (~3 cycles) is compensated in software. The
 * clock divider targets 1 cycle = 1 µs. Verify timing on a scope / receiver.
 */
#if defined(ARDUINO_ARCH_RP2040) && defined(VH_RF_PIO)

#include <hardware/pio.h>
#include <hardware/dma.h>
#include <hardware/clocks.h>

#ifndef RF_PIO_MAX
#define RF_PIO_MAX 4096            // max transitions per replay (uint32 word each)
#endif

class RfOokPio {

  public:
    // One-time setup: claim a PIO SM + DMA channel, load the program, route GDO0.
    void begin(const uint8_t gdo0)
    {
        // hand-assembled program (1 cycle = 1 us via clkdiv):
        //   0: out  pins, 1   ; GDO0 <- level bit (LSB of the FIFO word)
        //   1: out  x, 31     ; duration (cycles) <- next 31 bits
        //   2: jmp  x--, 2    ; hold: loop x+1 cycles
        // autopull @32, shift right. wrap 0..2.
        static const uint16_t INSNS[] = { 0x6001, 0x603f, 0x0042 };
        static const struct pio_program PROG = { .instructions = INSNS, .length = 3, .origin = -1 };

        this->_pio = pio0;
        this->_sm  = (uint)pio_claim_unused_sm(this->_pio, true);
        this->_off = (uint)pio_add_program(this->_pio, &PROG);

        pio_gpio_init(this->_pio, gdo0);
        pio_sm_set_consecutive_pindirs(this->_pio, this->_sm, gdo0, 1, true);

        pio_sm_config c = pio_get_default_sm_config();
        sm_config_set_wrap(&c, this->_off + 0, this->_off + 2);
        sm_config_set_out_pins(&c, gdo0, 1);
        sm_config_set_out_shift(&c, true /*shift right*/, true /*autopull*/, 32);
        const float div = (float)clock_get_hz(clk_sys) / 1000000.0f;   // -> 1 MHz
        sm_config_set_clkdiv(&c, div);
        pio_sm_init(this->_pio, this->_sm, this->_off, &c);
        pio_sm_set_enabled(this->_pio, this->_sm, true);

        this->_dma = (uint)dma_claim_unused_channel(true);
        this->_ready = true;
    }

    bool ready(void) const { return this->_ready; }

    // Start the DMA-driven playback of `n` signed-µs timings (non-blocking).
    void send(const SubfileDuration* t, size_t n)
    {
        if (!this->_ready) { return; }
        if (n > RF_PIO_MAX) { n = RF_PIO_MAX; }

        for (size_t i = 0; i < n; ++i) {
            const long d = (long)t[i];
            const uint32_t level = (d > 0) ? 1u : 0u;
            unsigned long us = (unsigned long)((d > 0) ? d : -d);
            // loop runs ~ (x+1) cycles + 2 for the two OUTs -> subtract 3
            const uint32_t cyc = (us > 3UL) ? (uint32_t)(us - 3UL) : 0u;
            this->_words[i] = (level & 1u) | (cyc << 1);
        }

        dma_channel_config dc = dma_channel_get_default_config(this->_dma);
        channel_config_set_transfer_data_size(&dc, DMA_SIZE_32);
        channel_config_set_read_increment(&dc, true);
        channel_config_set_write_increment(&dc, false);
        channel_config_set_dreq(&dc, pio_get_dreq(this->_pio, this->_sm, true));
        dma_channel_configure(this->_dma, &dc, &this->_pio->txf[this->_sm],
                              this->_words, n, true /*start now*/);
    }

    // True while the waveform is still being emitted (DMA + FIFO not drained).
    bool busy(void) const
    {
        if (!this->_ready) { return false; }
        return dma_channel_is_busy(this->_dma)
            || !pio_sm_is_tx_fifo_empty(this->_pio, this->_sm);
    }

  protected:
    PIO      _pio = nullptr;
    uint     _sm  = 0;
    uint     _off = 0;
    uint     _dma = 0;
    bool     _ready = false;
    uint32_t _words[RF_PIO_MAX];   // DMA source: [level:1][duration-3:31]
};

#endif // ARDUINO_ARCH_RP2040 && VH_RF_PIO
#endif
