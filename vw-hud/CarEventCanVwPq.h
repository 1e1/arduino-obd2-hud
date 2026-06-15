#ifndef CarEventCanVwPq_H_
#define CarEventCanVwPq_H_

#include <Arduino.h>
#include "CarEvent.h"   // aggregator: guarantees CarEvent + CarEventCan are defined first
#include "_can.h"
#include "_vwpq_decode.h"


/**
 * VW PQ (Sharan TSI 1.5) decode backend over the generic MCP2515 transport.
 *
 * This subclass holds the MCP2515-SPECIFIC knowledge only:
 *  - _updateMode(): per-mode MCP2515 acceptance filters keyed on the PQ
 *    arbitration IDs (see _can.h).
 *
 * The PQ DECODE itself (the switch on the arbitration id, canExtractLE signal
 * extraction, the native-unit members and _emit mapping) lives in the shared,
 * transport-agnostic VwPqDecode (_vwpq_decode.h) so it is NOT duplicated between
 * this MCP2515 backend and the ESP32 native-TWAI backend (CarEventTwaiVwPq).
 * _decodeFrame()/_emit() here are thin delegations into that shared core.
 *
 * The generic CarEventCan owns the transport, the pending queue and _value; it
 * carries no PQ knowledge.  Header-only like the other bench backends.
 *
 * !!! IDs / bit offsets / scales are in _can.h and are TO BE CONFIRMED ON BENCH
 *     (discodb2) against this exact car. !!!
 */
class CarEventCanVwPq : public CarEventCan {

    protected:
    void _updateMode(void) {
        // Exact 11-bit ID acceptance filters (standard frames, mask = 0x7FF).
        // RXB0: mask0 + filt0,filt1 ; RXB1: mask1 + filt2..filt5.
        // TODO confirm init_Mask/init_Filt signature against the installed mcp_can lib.
        this->_board->init_Mask(0, 0, 0x7FFUL);
        this->_board->init_Mask(1, 0, 0x7FFUL);

        switch (this->_mode) {
            case CarEvent::MODE_IDLE:
                // RPM (to detect engine start), Kombi_1 (fuel/handbrake), odometer, ignition
                this->_board->init_Filt(0, 0, VH_CAN_ID_MOTOR_1);
                this->_board->init_Filt(1, 0, VH_CAN_ID_KOMBI_1);
                this->_board->init_Filt(2, 0, VH_CAN_ID_KOMBI_3);
                this->_board->init_Filt(3, 0, VH_CAN_ID_ZAS_1);
                this->_board->init_Filt(4, 0, VH_CAN_ID_ZAS_1);   // spare -> harmless duplicate
                this->_board->init_Filt(5, 0, VH_CAN_ID_ZAS_1);
                break;

            case CarEvent::MODE_DRIVING:
            case CarEvent::MODE_DRIVING_SPORT:
                this->_board->init_Filt(0, 0, VH_CAN_ID_MOTOR_1);
                this->_board->init_Filt(1, 0, VH_CAN_ID_KOMBI_1);
                this->_board->init_Filt(2, 0, VH_CAN_ID_MOTOR_5);
                this->_board->init_Filt(3, 0, VH_CAN_ID_KOMBI_3);
                this->_board->init_Filt(4, 0, VH_CAN_ID_GETRIEBE_2);
                this->_board->init_Filt(5, 0, VH_CAN_ID_ZAS_1);
                break;

            default:
                break;
        }
    };

    void _decodeFrame(void) {
        VWPQ_DECODE_FRAME_BODY;
    };

    void _emit(const Sensor sensor) {
        VWPQ_EMIT_BODY;
    };

    // latest decoded raw values (native units, PQ-specific) — declared via the
    // shared macro so the TWAI backend declares the identical set.
    VWPQ_DECODE_MEMBERS;

};


#endif
