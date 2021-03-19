#ifndef HudisplayStream_HPP_
#define HudisplayStream_HPP_

#include <Arduino.h>
#include "Hudisplay.h"


class HudisplayStream : public Hudisplay {

  public:
  void setStream(Stream &stream) { this->_stream = &stream; };

  protected:
  void _switchOn(void) { this->_stream->println(F("* switchOn")); };
  void _switchOff(void) { this->_stream->println(F("* switchOfF(")); };

  void _update(void) {
    switch (this->_page) {
      case Hudisplay::PAGE_DRIVING:
        this->_stream->print(F("tankCapacity (l)="));
        this->_stream->println(this->_getTankCapacity());
        this->_stream->print(F("tankLoad (%256)="));
        this->_stream->println(this->_getTankLoad());
        this->_stream->print(F("tankValue (l)="));
        this->_stream->println(this->_getTankValue());
        this->_stream->print(F("consumptionLoad (%256)="));
        this->_stream->println(this->_getConsumptionLoad());
        this->_stream->print(F("consumptionValue (l)="));
        this->_stream->println(this->_getConsumptionInL());
        this->_stream->print(F("odometer (hm)="));
        this->_stream->println(this->_getOdometerValue());
        this->_stream->print(F("rpm (/64)="));
        this->_stream->println(this->_getRpmValueInDiv64());
        this->_stream->print(F("speed (km/h)="));
        this->_stream->println(this->_getSpeedValue());
        this->_stream->print(F("fuelConsumption (?)="));
        this->_stream->println(this->_getFuelConsumptionValue());
        break;

      case Hudisplay::PAGE_IDLE:
        this->_stream->print(F("avg consumption (dl/100km)="));
        this->_stream->println(this->_getAverageConsumptionInDlp100km());
        this->_stream->print(F("avg speed (km/h)="));
        this->_stream->println(this->_getAverageSpeedInKmph());
        break;
    }

    this->_stream->print(F("distance (hm)="));
    this->_stream->println(this->_getDistance());
    this->_stream->print(F("duration (s)="));
    this->_stream->println(this->_getDuration());

    this->_stream->println(F("-------"));
  };

  Stream* _stream = nullptr;

};


#endif
