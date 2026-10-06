#ifndef RfRemoteStream_HPP_
#define RfRemoteStream_HPP_

#include "RfRemote.h"

/**
 * Bench backend for the rf-replay add-on: prints the "transmit" to a Stream
 * instead of driving a CC1101. Lets you validate the whole button -> select ->
 * play flow with NO RF hardware (exactly how HudisplayStream / CarEventStream
 * validate their layers). Phase 3 adds the real RfRemoteCc1101 beside this.
 *
 * Usage (bench):
 *   RfRemoteStream rf;
 *   rf.setStream(Serial);
 *   rf.setButtons(VH_BTN_MENU, VH_BTN_EXEC, VH_BTN_DEBOUNCE, VH_RF_LABEL_MS);
 *   rf.setCount(2);            // two captured portals (phase 4: from LittleFS)
 *   rf.begin();
 *   ... rf.poll(now) each loop ...
 */
class RfRemoteStream : public RfRemote {

  public:
    void setStream(Stream& stream) { this->_out = &stream; }

  protected:
    void _transmitStart(const uint8_t index) override   // synchronous (prints, returns)
    {
        if (this->_out == nullptr) { return; }
        this->_out->print(F("[RF] TX portal "));
        this->_out->print((int)(index + 1));
        this->_out->print('/');
        this->_out->println((int)this->count());
    }

    Stream* _out = nullptr;
};


#endif
