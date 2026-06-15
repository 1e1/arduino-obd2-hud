#ifndef vwpq_decode_H_
#define vwpq_decode_H_

#include <Arduino.h>
#include "CarEvent.h"
#include "_can.h"


/**
 * Transport-agnostic VW PQ (Sharan TSI 1.5) decode core — SHARED, header-only.
 *
 * This is the SINGLE source of PQ decode truth, used verbatim by BOTH:
 *   - CarEventCanVwPq    (MCP2515 SPI transport, every board with a module)
 *   - CarEventTwaiVwPq   (ESP32 native TWAI controller, ARDUINO_ARCH_ESP32)
 *
 * Sharing strategy — token-identical paste, NOT a sub-object:
 *   A sub-object/base-class for the decode would shift the PQ members to a
 *   non-zero offset and change AVR address arithmetic (the ATmega328 baseline
 *   must stay BYTE-identical).  Instead the three pieces of PQ knowledge are
 *   expanded as macros directly inside each transport's own class body, so the
 *   compiler sees exactly the same tokens — direct `this->_member` access and a
 *   direct `this->_queue(...)` call — that the previous hand-written
 *   CarEventCanVwPq emitted.  The TWAI backend pastes the identical macros, so
 *   the decode switch / signal extraction / emit mapping is defined ONCE here.
 *
 * Each transport class body must paste, in this order:
 *   VWPQ_DECODE_FRAME_BODY   -> the body of _decodeFrame() (switch on _rxId)
 *   VWPQ_EMIT_BODY           -> the body of _emit(sensor)  (sensor -> _value)
 *   VWPQ_DECODE_MEMBERS      -> the native-unit member declarations
 * and must provide:
 *   - `this->_rxId`   (unsigned long)  the arbitration id of the read frame
 *   - `this->_buffer` (unsigned char[8]) the frame data
 *   - `this->_queue(Sensor)`           append a Sensor to the pending queue
 *   - `this->_value`  (unsigned long)  the emitted raw value sink
 *
 * !!! IDs / bit offsets / scales live in _can.h and are TO BE CONFIRMED ON BENCH
 *     (discodb2) against this exact car. !!!
 */


// --- _decodeFrame() body: decode by arbitration id, queue the carried Sensor(s) ---
#define VWPQ_DECODE_FRAME_BODY                                                                       \
    switch (this->_rxId) {                                                                           \
        case VH_CAN_ID_MOTOR_1:                                                                      \
            this->_rpmRaw = (unsigned short) canExtractLE(this->_buffer, VH_SIG_RPM_START, VH_SIG_RPM_LEN); \
            this->_queue(SENSOR_RPM);                                                                \
            break;                                                                                   \
        case VH_CAN_ID_KOMBI_1:                                                                      \
            this->_speedKmh   = (uint8_t)(canExtractLE(this->_buffer, VH_SIG_SPEED_START, VH_SIG_SPEED_LEN) / 100UL); \
            this->_tankLitres = (uint8_t) canExtractLE(this->_buffer, VH_SIG_TANK_START, VH_SIG_TANK_LEN);  \
            this->_handbrake  = (uint8_t) canExtractLE(this->_buffer, VH_SIG_HANDBRAKE_START, VH_SIG_HANDBRAKE_LEN); \
            this->_queue(SENSOR_VEHICLE_SPEED);                                                      \
            this->_queue(SENSOR_TANK_LOAD);                                                          \
            this->_queue(SENSOR_HANDBRAKE_POSITION);                                                 \
            break;                                                                                   \
        case VH_CAN_ID_MOTOR_5:                                                                      \
            this->_consCounter = (unsigned short) canExtractLE(this->_buffer, VH_SIG_CONS_START, VH_SIG_CONS_LEN); \
            this->_queue(SENSOR_FUEL_CONSUMPTION);                                                   \
            break;                                                                                   \
        case VH_CAN_ID_KOMBI_3:                                                                      \
            this->_odoKm = canExtractLE(this->_buffer, VH_SIG_ODO_START, VH_SIG_ODO_LEN);            \
            this->_queue(SENSOR_ODOMETER);                                                           \
            break;                                                                                   \
        case VH_CAN_ID_GETRIEBE_2:                                                                   \
            this->_gear = (uint8_t) canExtractLE(this->_buffer, VH_SIG_GEAR_START, VH_SIG_GEAR_LEN); \
            this->_queue(SENSOR_GEAR_POSITION);                                                      \
            break;                                                                                   \
        case VH_CAN_ID_ZAS_1:                                                                        \
            this->_ignition = (uint8_t) canExtractLE(this->_buffer, VH_SIG_IGNITION_START, VH_SIG_IGNITION_LEN); \
            this->_queue(SENSOR_IGNITION);                                                           \
            break;                                                                                   \
    }


// --- _emit() body: copy the emitted sensor's stored native value into _value ---
#define VWPQ_EMIT_BODY                                                                               \
    switch (sensor) {                                                                                \
        case SENSOR_RPM:                this->_value = this->_rpmRaw;      break;                     \
        case SENSOR_VEHICLE_SPEED:      this->_value = this->_speedKmh;    break;                     \
        case SENSOR_TANK_LOAD:          this->_value = this->_tankLitres;  break;                     \
        case SENSOR_HANDBRAKE_POSITION: this->_value = this->_handbrake;   break;                     \
        case SENSOR_FUEL_CONSUMPTION:   this->_value = this->_consCounter; break;                     \
        case SENSOR_ODOMETER:           this->_value = this->_odoKm;       break;                     \
        case SENSOR_GEAR_POSITION:      this->_value = this->_gear;        break;                     \
        case SENSOR_IGNITION:           this->_value = this->_ignition;    break;                     \
        default:                        this->_value = 0;                  break;                     \
    }


// --- native-unit decoded members (PQ-specific) ---
#define VWPQ_DECODE_MEMBERS                                                                          \
    unsigned short _rpmRaw = 0;        /* rpm*4 */                                                   \
    uint8_t        _speedKmh = 0;      /* km/h */                                                    \
    uint8_t        _tankLitres = 0;    /* L */                                                       \
    uint8_t        _handbrake = 0;     /* 0/1 */                                                     \
    unsigned short _consCounter = 0;   /* µl (15-bit, rolling) */                                    \
    unsigned long  _odoKm = 0;         /* km */                                                      \
    uint8_t        _gear = 0;                                                                        \
    uint8_t        _ignition = 0       /* 0/1 ; trailing ';' supplied at paste site */


#endif
