#include "CarEvent.h"   // aggregator: defines CarEvent, then CarEventCan + backends (avoids include cycle)



// ==============================================
// PUBLIC
// ==============================================


void CarEventCan::setBoard(MCP_CAN* board)
{
    this->_board = board;
    this->_board->setSleepWakeup(1);   // wake on bus activity
}


// ==============================================
// PROTECTED
// ==============================================


void CarEventCan::_switchOn(void)
{
    this->_bufferLength = 0;
    this->_pendingCount = 0;
    this->_pendingIndex = 0;
    this->_board->setMode(MCP_LISTENONLY);
}


void CarEventCan::_switchOff(void)
{
    this->_board->setMode(MCP_SLEEP);
}


void CarEventCan::_update(void)
{
    // 1) drain signals already decoded from the previous frame;
    //    otherwise read and decode a new frame.
    if (this->_pendingIndex >= this->_pendingCount) {
        if (CAN_OK == this->_board->readMsgBuf(&this->_rxId, &this->_bufferLength, this->_buffer)) {
            this->_pendingCount = 0;
            this->_pendingIndex = 0;
            this->_decodeFrame();   // vehicle-specific: fills members + _queue()
        } else {
            return;   // no frame: _sensor stays SENSOR_NONE (set by CarEvent::update)
        }
    }

    // 2) emit the next pending signal with its stored native value.
    if (this->_pendingIndex < this->_pendingCount) {
        const Sensor sensor = this->_pending[this->_pendingIndex++];
        this->_sensor = sensor;
        this->_emit(sensor);        // vehicle-specific: store value into _value
    }
}


void CarEventCan::_queue(const Sensor sensor)
{
    if (this->_pendingCount < PENDING_MAX) {
        this->_pending[this->_pendingCount++] = sensor;
    }
}


uint8_t CarEventCan::_readByte(void) const
{
    return (uint8_t)(this->_value & 0xFF);
}


unsigned short CarEventCan::_readShort(void) const
{
    return (unsigned short)(this->_value & 0xFFFF);
}


unsigned long CarEventCan::_readLong(void) const
{
    return this->_value;
}
