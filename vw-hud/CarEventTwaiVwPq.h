#ifndef CarEventTwaiVwPq_H_
#define CarEventTwaiVwPq_H_

#if defined(ARDUINO_ARCH_ESP32)

#include <Arduino.h>
#include "driver/twai.h"   // ESP-IDF legacy TWAI driver, re-exported by arduino-esp32
#include "CarEvent.h"      // aggregator: guarantees CarEvent is defined first
#include "_can.h"
#include "_vwpq_decode.h"
#include "_wiring.h"


/**
 * VW PQ decode backend over the ESP32 NATIVE TWAI controller (no MCP2515).
 *
 * The XIAO ESP32-C3 has an on-die TWAI (CAN 2.0) controller, so it needs a CAN
 * transceiver only (e.g. SN65HVD230 / TJA1051) — NOT an MCP2515 SPI module.
 * This backend therefore does NOT derive from CarEventCan (which is the MCP2515
 * SPI transport); it implements the same CarEvent transport contract directly on
 * top of the ESP-IDF legacy TWAI driver (driver/twai.h).
 *
 * The PQ DECODE (switch on the arbitration id, canExtractLE extraction, emit
 * mapping, native-unit members) is SHARED VERBATIM with the MCP2515 backend via
 * the macros in _vwpq_decode.h — it is defined exactly once, NOT duplicated.
 *
 * TWAI driver configuration (ESP-IDF legacy `driver/twai.h`, documented at
 * docs.espressif.com .../peripherals/twai.html):
 *   - g_config = TWAI_GENERAL_CONFIG_DEFAULT(tx_io, rx_io, TWAI_MODE_LISTEN_ONLY)
 *       LISTEN_ONLY: a sniffer HUD must never ACK / influence the bus.
 *   - t_config = TWAI_TIMING_CONFIG_500KBITS()  (VW PQ powertrain = 500 kbit/s)
 *   - f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL() (hardware accept-all; the PQ ids
 *       we care about are selected in software by the decode switch — unmatched
 *       ids fall through harmlessly, exactly like the MCP filters' superset).
 *   - twai_receive(&msg, 0): non-blocking poll, same cadence as the MCP read loop.
 *
 * GPIO (VH_TWAI_TX / VH_TWAI_RX, defined in _wiring.h) are a BENCH-GUESS and must
 * be verified against the wired transceiver.
 *
 * !!! IDs / bit offsets / scales are in _can.h and are TO BE CONFIRMED ON BENCH
 *     (discodb2) against this exact car. !!!
 */
class CarEventTwaiVwPq : public CarEvent {

    public:
    // Install + start the native TWAI driver. Call once from setup() (mirrors the
    // role of MCP_CAN::begin + CarEventCan::setBoard on the MCP2515 boards).
    void begin(void) {
        twai_general_config_t g_config =
            TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)VH_TWAI_TX, (gpio_num_t)VH_TWAI_RX, TWAI_MODE_LISTEN_ONLY);
        twai_timing_config_t  t_config = TWAI_TIMING_CONFIG_500KBITS();   // VH_CAN_BITRATE == CAN_500KBPS
        twai_filter_config_t  f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

        if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
            this->_installed = true;
            twai_start();
        }
    };

    protected:
    static const uint8_t PENDING_MAX = 4;

    void _updateMode(void) {
        // Hardware filter is accept-all; per-mode id selection is done in software
        // by the decode switch (unmatched ids ignored). No TWAI reconfig needed.
    };

    void _switchOn(void) {
        this->_bufferLength = 0;
        this->_pendingCount = 0;
        this->_pendingIndex = 0;
        if (this->_installed) {
            twai_start();   // idempotent if already running
        }
    };

    void _switchOff(void) {
        if (this->_installed) {
            twai_stop();
        }
    };

    void _update(void) {
        // 1) drain signals already decoded from the previous frame; otherwise poll
        //    one raw frame (non-blocking, 0 ticks) and decode it. Same shape as
        //    CarEventCan::_update so the one-sensor-per-update() contract holds.
        if (this->_pendingIndex >= this->_pendingCount) {
            twai_message_t msg;
            if (this->_installed && twai_receive(&msg, 0) == ESP_OK) {
                this->_pendingCount = 0;
                this->_pendingIndex = 0;
                this->_rxId = msg.identifier;
                this->_bufferLength = msg.data_length_code;
                for (uint8_t i = 0; i < 8; ++i) {
                    this->_buffer[i] = (i < msg.data_length_code) ? msg.data[i] : 0;
                }
                this->_decodeFrame();
            } else {
                return;   // no frame: _sensor stays SENSOR_NONE (set by CarEvent::update)
            }
        }

        // 2) emit the next pending signal with its stored native value.
        if (this->_pendingIndex < this->_pendingCount) {
            const Sensor sensor = this->_pending[this->_pendingIndex++];
            this->_sensor = sensor;
            this->_emit(sensor);
        }
    };

    uint8_t        _readByte(void)  const { return (uint8_t)(this->_value & 0xFF); };
    unsigned short _readShort(void) const { return (unsigned short)(this->_value & 0xFFFF); };
    unsigned long  _readLong(void)  const { return this->_value; };

    void _queue(const Sensor sensor) {
        if (this->_pendingCount < PENDING_MAX) {
            this->_pending[this->_pendingCount++] = sensor;
        }
    };

    // --- PQ decode + emit, SHARED VERBATIM with CarEventCanVwPq (_vwpq_decode.h) ---
    void _decodeFrame(void) {
        VWPQ_DECODE_FRAME_BODY;
    };

    void _emit(const Sensor sensor) {
        VWPQ_EMIT_BODY;
    };

    // transport state
    bool _installed = false;
    unsigned long _rxId = 0;
    unsigned char _buffer[8];
    uint8_t _bufferLength = 0;
    unsigned long _value = 0;

    Sensor _pending[PENDING_MAX];
    uint8_t _pendingCount = 0;
    uint8_t _pendingIndex = 0;

    // latest decoded raw values (native units) — identical set to CarEventCanVwPq
    VWPQ_DECODE_MEMBERS;

};


#endif // ARDUINO_ARCH_ESP32
#endif
