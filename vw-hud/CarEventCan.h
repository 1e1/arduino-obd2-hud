#ifndef CarEventCan_H_
#define CarEventCan_H_

#include <Arduino.h>
#include <mcp_can.h>
#include "CarEvent.h"


/**
 * Generic MCP2515 CAN transport (ABSTRACT — no vehicle knowledge).
 *
 * Responsibilities (vehicle-agnostic):
 *  - own the MCP_CAN board + LISTENONLY/SLEEP power switching (setBoard,
 *    _switchOn/_switchOff);
 *  - read one raw frame per _update() (readMsgBuf), then drain the per-frame
 *    pending-signal queue one signal per update() so the one-sensor-per-update()
 *    contract of CarEvent is preserved;
 *  - expose the decoded scalar through _value via _readByte/_readShort/_readLong.
 *
 * What a vehicle subclass MUST provide (pure virtual):
 *  - _updateMode(): program the hardware acceptance filters for the current Mode;
 *  - _decodeFrame(): decode the signals of the just-read frame (_rxId/_buffer)
 *    into its own members and _queue() the corresponding Sensor(s);
 *  - _emit(Sensor): copy the stored native value of that Sensor into _value.
 *
 * Helpers offered to subclasses: _queue(Sensor) (append to the pending list)
 * and the protected _rxId/_buffer/_bufferLength/_value fields.
 *
 * mcp_can API targeted: coryjfowler (init_Mask/init_Filt 3-arg, setMode,
 * readMsgBuf 3-arg). To confirm against the installed library.
 */
class CarEventCan : public CarEvent {
    public:
    void setBoard(MCP_CAN* board);

    protected:
    static const uint8_t PENDING_MAX = 4;

    // transport (generic)
    void _switchOn(void);
    void _switchOff(void);
    void _update(void);

    uint8_t _readByte(void) const;
    unsigned short _readShort(void) const;
    unsigned long _readLong(void) const;

    void _queue(const Sensor sensor);

    // vehicle-specific (provided by the subclass)
    virtual void _updateMode(void) = 0;     // inherited pure-virtual from CarEvent
    virtual void _decodeFrame(void) = 0;    // decode _rxId/_buffer -> members + _queue
    virtual void _emit(const Sensor sensor) = 0;  // store the sensor's value into _value

    MCP_CAN* _board = nullptr;
    unsigned long _rxId = 0;
    unsigned char _buffer[8];
    uint8_t _bufferLength = 0;

    unsigned long _value = 0;          // raw value of the sensor currently emitted

    // signals decoded from the last frame, emitted one per update()
    Sensor _pending[PENDING_MAX];
    uint8_t _pendingCount = 0;
    uint8_t _pendingIndex = 0;
};


#endif
