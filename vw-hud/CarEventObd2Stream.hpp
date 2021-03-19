#ifndef CarEventObd2Stream_HPP_
#define CarEventObd2Stream_HPP_

#include <Arduino.h>
#include "CarEventObd2.h"


class CarEventObd2Stream : public CarEventObd2 {

  public:
  void setStream(Stream &stream) { this->_stream = &stream; };

  protected:
  void _updateMode(void) {};
  void _switchOn(void) { this->_stream->println(F("* switchOn")); };
  void _switchOff(void) { this->_stream->println(F("* switchOff")); };

  void _update(void) {
    const uint8_t sensorId = this->_stream->parseInt();
    const unsigned long sensorValue = this->_stream->parseInt();

    switch (sensorId) {
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
        this->_buffer[0] = sensorValue >> 24;
        this->_buffer[1] = sensorValue >> 16;
        this->_buffer[2] = sensorValue >> 8;
        this->_buffer[3] = sensorValue >> 0;
        break;
      case 4: 
        this->_sensor = SENSOR_RPM; 
        this->_bufferLength = 2;
        this->_buffer[0] = sensorValue >> 8;
        this->_buffer[1] = sensorValue >> 0;
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

  Stream* _stream = nullptr;

};


#endif
