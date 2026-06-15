#ifndef HudisplayStream_HPP_
#define HudisplayStream_HPP_

#include <Arduino.h>
#include "Hudisplay.h"


class HudisplayStream : public Hudisplay {

  public:
  static const uint8_t HISTORY_FRAME_SIZE = 8;
  static const uint8_t FRAME_DURATION_MS = 255;
  static const uint8_t FRAME_NB_SKIP = 4;

  void setStream(Stream &stream) { this->_stream = &stream; this->_stream->println(F("HudisplayStream: OK")); this->_stream->flush(); };

  protected:
  void _switchOn(void)  { this->_stream->println(F("* switchOn" )); this->_stream->flush(); };
  void _switchOff(void) { this->_stream->println(F("* switchOff")); this->_stream->flush(); };
  void _setNightMode(const bool isNight) { this->_stream->print(F("* isNight=")); this->_stream->println(isNight); this->_stream->flush(); };

  void _update(void) {
    if (0 == this->_frameIndex) {
      switch (this->_page) {
        case Hudisplay::PAGE_DRIVING:
          // native PQ-CAN units (post-OBD pivot): litres / km / l-per-100 / µl
          this->_stream->print(F("tankCapacity       (l)="));
          this->_stream->println(this->_getTankCapacity());
          this->_stream->print(F("tankLoad           (l)="));
          this->_stream->println(this->_getTankLoad());
          this->_stream->print(F("tankValue          (l)="));
          this->_stream->println(this->_getTankValue());
          this->_stream->print(F("consumptionLoad    (l)="));
          this->_stream->println(this->_getConsumptionLoad());
          this->_stream->print(F("consumptionValue   (l)="));
          this->_stream->println(this->_getConsumptionInL());
          this->_stream->print(F("odometer          (km)="));
          this->_stream->println(this->_getOdometerValue());
          this->_stream->print(F("rpm              (/64)="));
          this->_stream->println(this->_getRpmValueInDiv64());
          this->_stream->print(F("speed           (km/h)="));
          this->_stream->println(this->_getSpeedValue());
          this->_stream->print(F("fuelConsumption (ringB)="));
          this->_stream->println(this->_getFuelConsumptionValue());
          this->_stream->print(F("gearPosition       (?)="));
          this->_stream->println(this->_getGearPosition());
          break;

        case Hudisplay::PAGE_IDLE:
          this->_stream->print(F("avg consumption (l/100km)="));
          this->_stream->println(this->_getAverageConsumptionInLp10km());
          this->_stream->print(F("avg speed          (km/h)="));
          this->_stream->println(this->_getAverageSpeedInKmh());
          break;

        case Hudisplay::PAGE_BOOT:
        case Hudisplay::PAGE_NONE:
          // no scalar dump for the boot starfield / idle-none page
          break;
      }

      this->_stream->print(F("distance (km)="));
      this->_stream->println(this->_getDistance());
      this->_stream->print(F("duration (4s)="));
      this->_stream->println(this->_getDuration());

      this->_stream->println(F("-------"));
      this->_stream->flush();
    }
  };

  Stream* _stream = nullptr;

};


#endif
