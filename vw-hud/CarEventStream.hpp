#ifndef CarEventStream_HPP_
#define CarEventStream_HPP_

#include <Arduino.h>
#include "CarEvent.h"


/**
 * Bench / simulator backend: sensor values are injected over a Stream (Serial)
 * as plain-text "<sensorId> <value>\n" pairs, one sensor per line. The PC-side
 * simulator (www/) writes these frames so the *real* Hudisplay renders exactly
 * what it would from the car.
 *
 * Wire protocol: <sensorId> is the CarEvent::Sensor enum ordinal (see CarEvent.h),
 * <value> is the native unit expected by the matching getter. Ordinals (current
 * cleaned enum — OBD-era MAF/ENGINE_FUEL_RATE/TORQUE removed):
 *
 *   2 = TANK_CAPACITY  (litres)      7  = FUEL_CONSUMPTION (cumulative µl)
 *   3 = TANK_LOAD      (litres 0-126) 8 = HANDBRAKE_POSITION (0/1)
 *   4 = ODOMETER       (km)          9  = GEAR_POSITION      (cluster code)
 *   5 = RPM            (raw rpm*4)   10 = IGNITION           (0/1, Klemme_15)
 *   6 = VEHICLE_SPEED  (km/h)
 *
 *   (0 = NONE, 1 = TIMEOUT are reserved control ordinals, not data.)
 *
 * IGNITION drives the whole Workflow state machine: without a "10 1" frame the
 * screen never turns on.
 *
 * Same one-sensor-per-update() contract and _value accumulator as CarEventCan.
 */
class CarEventStream : public CarEvent {

  public:
  void setStream(Stream &stream) {
    this->_stream = &stream;
    this->_stream->setTimeout(20);          // don't block loop() waiting for a full line
    this->_stream->println(F("CarEventStream: OK"));
    this->_stream->flush();
  };

  protected:
  void _updateMode(void) {};
  void _switchOn(void)  { this->_stream->println(F("* switchOn" )); this->_stream->flush(); };
  void _switchOff(void) { this->_stream->println(F("* switchOff")); this->_stream->flush(); };

  void _update(void) {
    if (this->_stream->available() <= 0) {
      return;                               // no data -> _sensor stays SENSOR_NONE
    }

    const long sensorId = this->_stream->parseInt();
    this->_value        = (unsigned long) this->_stream->parseInt();

    // wire id == Sensor enum ordinal; ignore anything outside the live range
    if (sensorId > SENSOR_NONE && sensorId < SENSOR_COUNT) {
      this->_sensor = (Sensor) sensorId;
    }
  };

  uint8_t        _readByte(void)  const { return (uint8_t)(this->_value & 0xFF); };
  unsigned short _readShort(void) const { return (unsigned short)(this->_value & 0xFFFF); };
  unsigned long  _readLong(void)  const { return this->_value; };

  Stream* _stream = nullptr;
  unsigned long _value = 0;

};


#endif
