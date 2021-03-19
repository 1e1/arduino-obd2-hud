#ifndef CarEventHardware_HPP_
#define CarEventHardware_HPP_

#include <Arduino.h>
#include "CarEvent.h"


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
  void _updateMode(void) { this->_state = 0; };
  void _switchOn(void) {};
  void _switchOff(void) {};

  void _update(void) {
    bool b = digitalRead(this->_pinButton);
    const uint8_t sensorValue = map(analogRead(this->_pinVariac), 0, 1024, 0, 256);

    if (b != this->_stateButton) {
      this->_stateButton = b;

      if (this->_stateButton) {
        ++this->_state;
        if (this->_state >= this->SENSOR_COUNT) {
          this->_state = 0;
        }
      }
    }

    switch (this->_state) {
      case 1:
        this->_sensor = SENSOR_TANK_CAPACITY;
        this->_sensorValue = map(sensorValue, 0, 1024, 0, 256);
        break;
      case 2:
        this->_sensor = SENSOR_TANK_LOAD;
        this->_sensorValue = map(sensorValue, 0, 1024, 0, 256);
        break;
      case 3:
        this->_sensor = SENSOR_ODOMETER;
        this->_sensorValue = map(sensorValue, 0, 1024, 0, 65536) * 65536;
        break;
      case 4:
        this->_sensor = SENSOR_RPM;
        this->_sensorValue = map(sensorValue, 0, 1024, 0, 65536);
        break;
      case 5:
        this->_sensor = SENSOR_TORQUE_LOAD;
        this->_sensorValue = map(sensorValue, 0, 1024, 0, 256);
        break;
      case 6:
        this->_sensor = SENSOR_VEHICLE_SPEED;
        this->_sensorValue = map(sensorValue, 0, 1024, 0, 256);
        break;
      case 7:
        this->_sensor = SENSOR_FUEL_CONSUMPTION;
        this->_sensorValue = map(sensorValue, 0, 1024, 0, 256);
        break;
      case 8:
        this->_sensor = SENSOR_HANDBRAKE_POSITION;
        this->_sensorValue = map(sensorValue, 0, 1024, 0, 256);
        break;
      case 9:
        this->_sensor = SENSOR_GEAR_POSITION;
        this->_sensorValue = map(sensorValue, 0, 1024, 0, 256);
        break;
    }
  };


  const uint8_t _readByte(void) const
  {
      return this->_sensorValue;
  };


  const unsigned short _readShort(void) const
  {
      return this->_sensorValue;
  };


  const unsigned long _readLong(void) const
  {
      return this->_sensorValue;
  };


  uint8_t _pinButton;
  uint8_t _pinVariac;
  bool _stateButton;
  uint8_t _state;
  unsigned long _sensorValue;

};


#endif
