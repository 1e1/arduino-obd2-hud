#ifndef CarEventStream_HPP_
#define CarEventStream_HPP_

#include <Arduino.h>
#include "CarEvent.h"


class CarEventStream : public CarEvent {

  public:
  void setStream(Stream &stream) { this->_stream = &stream; this->_stream->println(F("CarEventStream: OK")); this->_stream->flush(); };

  protected:
  void _updateMode(void) {};
  void _switchOn(void)  { this->_stream->println(F("* switchOn" )); this->_stream->flush(); };
  void _switchOff(void) { this->_stream->println(F("* switchOff")); this->_stream->flush(); };

  void _update(void) {
    if (this->_stream->available() > 0) {
      const uint8_t sensorId = this->_stream->parseInt();
      this->_sensorValue = this->_stream->parseInt();
      /*
      this->_stream->flush();
      this->_stream->print("sensorId=");
      this->_stream->println(sensorId);
      this->_stream->print("_sensorValue=");
      this->_stream->println(this->_sensorValue);
      this->_stream->flush();
      */
      switch (sensorId) {
        case 1: 
          this->_sensor = SENSOR_TANK_CAPACITY; 
          break;
        case 2: 
          this->_sensor = SENSOR_TANK_LOAD; 
          break;
        case 3: 
          this->_sensor = SENSOR_ODOMETER; 
          break;
        case 4: 
          this->_sensor = SENSOR_RPM; 
          break;
        case 5: 
          this->_sensor = SENSOR_MAF; 
          break;
        case 6: 
          this->_sensor = SENSOR_TORQUE_LOAD; 
          break;
        case 7: 
          this->_sensor = SENSOR_VEHICLE_SPEED; 
          break;
        case 8: 
          this->_sensor = SENSOR_FUEL_CONSUMPTION; 
          break;
        case 9: 
          this->_sensor = SENSOR_HANDBRAKE_POSITION;
          break;
        case 10: 
          this->_sensor = SENSOR_GEAR_POSITION; 
          break;
      }
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


  Stream* _stream = nullptr;
  unsigned long _sensorValue;

};


#endif
