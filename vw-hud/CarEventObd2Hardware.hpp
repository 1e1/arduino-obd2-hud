#ifndef CarEventObd2Hardware_HPP_
#define CarEventObd2Hardware_HPP_

#include <Arduino.h>
#include "CarEventObd2.h"


class CarEventObd2Hardware : public CarEventObd2 {

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
        this->_bufferLength = 1;
        this->_buffer[0] = sensorValue;
        break;
      case 2:
        this->_sensor = SENSOR_TANK_LOAD;
        this->_bufferLength = 1;
        this->_buffer[0] = sensorValue;
        break;
      case 3:
        this->_sensor = SENSOR_ODOMETER;
        this->_bufferLength = 4;
        this->_buffer[0] = sensorValue;
        this->_buffer[1] = sensorValue;
        this->_buffer[2] = sensorValue;
        this->_buffer[3] = sensorValue;
        break;
      case 4:
        this->_sensor = SENSOR_RPM;
        this->_bufferLength = 2;
        this->_buffer[0] = sensorValue;
        this->_buffer[1] = sensorValue;
        break;
      case 5:
        this->_sensor = SENSOR_TORQUE_LOAD;
        this->_bufferLength = 1;
        this->_buffer[0] = sensorValue;
        break;
      case 6:
        this->_sensor = SENSOR_VEHICLE_SPEED;
        this->_bufferLength = 1;
        this->_buffer[0] = sensorValue;
        break;
      case 7:
        this->_sensor = SENSOR_FUEL_CONSUMPTION;
        this->_bufferLength = 1;
        this->_buffer[0] = sensorValue;
        break;
      case 8:
        this->_sensor = SENSOR_HANDBRAKE_POSITION;
        this->_bufferLength = 1;
        this->_buffer[0] = sensorValue;
        break;
      case 9:
        this->_sensor = SENSOR_GEAR_POSITION;
        this->_bufferLength = 1;
        this->_buffer[0] = sensorValue;
        break;
    }
  };

  uint8_t _pinButton;
  uint8_t _pinVariac;
  bool _stateButton;
  uint8_t _state;

};


#endif
