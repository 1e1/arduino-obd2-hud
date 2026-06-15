#ifndef CarEventHardware_HPP_
#define CarEventHardware_HPP_

#include <Arduino.h>
#include "CarEvent.h"


/**
 * Bench backend: a push-button cycles the selected sensor, a potentiometer sets
 * its value. Slot 0 is IGNITION (Klemme_15) so the knob arms the contact and the
 * Workflow state machine can boot; the button then cycles the data sensors.
 *
 * Same one-sensor-per-update() contract, native units and _value accumulator as
 * CarEventCan (RPM = raw rpm*4, speed = km/h, tank = litres, odo = km).
 */
class CarEventHardware : public CarEvent {

  public:
  void setWiring(const uint8_t pinButton, const uint8_t pinVariac)
  {
    this->_pinButton = pinButton;
    this->_pinVariac = pinVariac;

    pinMode(pinButton, INPUT_PULLUP);
    pinMode(pinVariac, INPUT_PULLUP);
  };

  protected:
  // sensors exposed on the knob, in button-cycle order (slot 0 = ignition)
  static const uint8_t CYCLE_COUNT = 7;
  const Sensor _cycle[CYCLE_COUNT] = {
    SENSOR_IGNITION,
    SENSOR_RPM,
    SENSOR_VEHICLE_SPEED,
    SENSOR_TANK_LOAD,
    SENSOR_GEAR_POSITION,
    SENSOR_HANDBRAKE_POSITION,
    SENSOR_ODOMETER,
  };

  void _updateMode(void) { this->_state = 0; };
  void _switchOn(void) {};
  void _switchOff(void) {};

  void _update(void) {
    const bool button = digitalRead(this->_pinButton);
    const uint16_t raw = analogRead(this->_pinVariac);   // 0..1023

    if (button != this->_stateButton) {
      this->_stateButton = button;
      if (this->_stateButton && (++this->_state >= CYCLE_COUNT)) {
        this->_state = 0;
      }
    }

    const Sensor sensor = this->_cycle[this->_state];
    this->_sensor = sensor;

    switch (sensor) {
      case SENSOR_IGNITION:           this->_value = (raw > 512) ? 1UL : 0UL;                  break;
      case SENSOR_RPM:                this->_value = (unsigned long) map(raw, 0, 1023, 0, 8000) * 4UL; break; // raw = rpm*4 (bar full-scale 8192)
      case SENSOR_VEHICLE_SPEED:      this->_value = map(raw, 0, 1023, 0, 220);                break;
      case SENSOR_TANK_LOAD:          this->_value = map(raw, 0, 1023, 0, TANK_CAPACITY_MAX);  break;
      case SENSOR_GEAR_POSITION:      this->_value = map(raw, 0, 1023, 0, 9);                  break;
      case SENSOR_HANDBRAKE_POSITION: this->_value = (raw > 512) ? 1UL : 0UL;                  break;
      case SENSOR_ODOMETER:           this->_value = map(raw, 0, 1023, 0, 300000);             break; // km
      default:                        this->_sensor = SENSOR_NONE;                            break;
    }
  };

  uint8_t        _readByte(void)  const { return (uint8_t)(this->_value & 0xFF); };
  unsigned short _readShort(void) const { return (unsigned short)(this->_value & 0xFFFF); };
  unsigned long  _readLong(void)  const { return this->_value; };

  uint8_t _pinButton = 0;
  uint8_t _pinVariac = 0;
  bool _stateButton = false;
  uint8_t _state = 0;
  unsigned long _value = 0;

};


#endif
