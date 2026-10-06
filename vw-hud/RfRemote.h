#ifndef RfRemote_H_
#define RfRemote_H_

#include <Arduino.h>
#include "_wiring.h"
#include "_button.h"

/**
 * OPTIONAL rf-replay add-on — controller layer (see doc/SUBGHZ-FORMAT.md).
 *
 * Two buttons drive a tiny display state machine over the fuel-gauge band:
 *   B1 (menu, VH_BTN_MENU): gauge shown -> show the selected .sub
 *                           .sub shown  -> next .sub (becomes the selection)
 *   B2 (exec, VH_BTN_EXEC): show the selected .sub AND replay it INSTANTLY
 *                           (whether it was already shown or only in memory)
 *   no press for VH_RF_LABEL_MS -> the band reverts to the fuel gauge
 *
 * view() exposes GAUGE vs FILE so the renderer knows whether to draw the normal
 * fuel gauge or the selected .sub name in the band (y 54-63, profont12). The
 * emission itself is delegated to the backend `_transmit()`:
 *   - RfRemoteStream  (bench) — prints the TX to a Stream, no RF hardware.
 *   - RfRemoteCc1101  (phase 3) — real OOK bit-bang on the CC1101 (blocking,
 *     ~0.5-1 s; the display is modal during that window = the "EMISSION" state).
 *
 * NO long-press: B2 fires on a (debounced) press edge. The anti-accidental
 * protection is the standstill arming gate in Workflow (speed 0 + ignition +
 * handbrake), NOT a hold. Buttons are ACTIVE-LOW + internal pull-up (to GND);
 * see _button.h for why that is the car-robust choice.
 *
 * HEADER-ONLY (the Arduino toolchain compiles every sketch .cpp unconditionally;
 * inline-in-header + included only under VH_FEATURE_RF keeps the add-on truly
 * zero-cost when disabled).
 */
class RfRemote {

  public:
    typedef enum { VIEW_GAUGE, VIEW_FILE } View;

    void setButtons(const uint8_t pinMenu, const uint8_t pinExec,
                    const uint8_t debounceDepth, const uint32_t labelTimeoutMs)
    {
        this->_pinMenu = pinMenu;
        this->_pinExec = pinExec;
        this->_depth   = debounceDepth;
        this->_labelMs = labelTimeoutMs;
    }

    void begin(void)
    {
        pinMode(this->_pinMenu, INPUT_PULLUP);
        pinMode(this->_pinExec, INPUT_PULLUP);
        button_init(&this->_bMenu);
        button_init(&this->_bExec);
        this->_lastMs = 0;
        this->_view = VIEW_GAUGE;
    }

    // Loaded portals. setNames() provides the .sub display names (phase 4 feeds
    // this from the LittleFS list); setCount() alone is enough for a nameless
    // backend (selectedName() then returns ""). `names` must outlive this object.
    void    setNames(const char* const* names, const uint8_t count)
            { this->_names = names; this->_count = count; if (this->_sel >= count) this->_sel = 0; }
    void    setCount(const uint8_t n) { this->_count = n; if (this->_sel >= n) this->_sel = 0; }
    uint8_t count(void) const         { return this->_count; }
    uint8_t selected(void) const      { return this->_sel; }
    const char* selectedName(void) const
            { return (this->_names != nullptr && this->_sel < this->_count) ? this->_names[this->_sel] : ""; }
    View    view(void) const          { return this->_view; }   // GAUGE / FILE — renderer reads this

    // Sample both buttons once. `nowMs` = wall clock (Workflow passes
    // Energy.realMillis(), which is CPU-clock-scaling aware). Call every loop.
    void poll(const unsigned long nowMs)
    {
        // Emission in progress (non-blocking backend, e.g. PIO): let it finish,
        // ignore the buttons meanwhile. The view stays FILE (name on the band),
        // so the OLED keeps showing the portal while core0 renders normally.
        if (this->_emitting) {
            this->_lastMs = nowMs;
            if (!this->_transmitBusy()) {
                this->_transmitEnd();
                this->_emitting = false;
                this->_rearm(nowMs);       // start the inactivity timer now
            }
            return;
        }

        uint16_t dt = (uint16_t)(nowMs - this->_lastMs);
        if (dt > 1000) { dt = 1000; }          // clamp first call / post-sleep jump
        this->_lastMs = nowMs;

        button_update(&this->_bMenu, digitalRead(this->_pinMenu) == LOW, dt, this->_depth);
        button_update(&this->_bExec, digitalRead(this->_pinExec) == LOW, dt, this->_depth);

        if (button_pressEdge(&this->_bMenu)) { this->_menu(nowMs); }
        if (button_pressEdge(&this->_bExec)) { this->_exec(nowMs); }

        // inactivity -> revert to the fuel gauge
        if (this->_view == VIEW_FILE && (long)(nowMs - this->_deadline) >= 0) {
            this->_view = VIEW_GAUGE;
        }
    }

  protected:
    void _rearm(const unsigned long now) { this->_deadline = now + this->_labelMs; }

    void _menu(const unsigned long now)
    {
        if (this->_count == 0) { this->_view = VIEW_FILE; this->_rearm(now); return; }
        if (this->_view == VIEW_GAUGE) { this->_view = VIEW_FILE; }        // reveal the selection
        else { this->_sel = (uint8_t)((this->_sel + 1) % this->_count); }  // next, now selected
        this->_rearm(now);
    }

    void _exec(const unsigned long now)
    {
        if (this->_count == 0) { return; }
        this->_view = VIEW_FILE;
        this->_transmitStart(this->_sel);
        if (this->_transmitBusy()) { this->_emitting = true; }  // async: finished in poll()
        else { this->_rearm(now); }                             // sync: done now
    }

    // Backend emission hooks. A blocking backend (stream, plain CC1101 bit-bang)
    // does all the work in _transmitStart() and leaves _transmitBusy() false. A
    // non-blocking backend (CC1101 + PIO) kicks off in _transmitStart(), reports
    // _transmitBusy() while the waveform plays, and tidies up in _transmitEnd().
    virtual void _transmitStart(const uint8_t index) = 0;
    virtual bool _transmitBusy(void) { return false; }
    virtual void _transmitEnd(void) {}

    uint8_t  _pinMenu = 0;
    uint8_t  _pinExec = 0;
    uint8_t  _depth   = 4;
    uint32_t _labelMs = 4096;
    Button   _bMenu;
    Button   _bExec;
    unsigned long _lastMs   = 0;
    unsigned long _deadline = 0;
    const char* const* _names = nullptr;   // optional .sub display names (len = _count)
    uint8_t  _count = 0;
    uint8_t  _sel   = 0;
    View     _view  = VIEW_GAUGE;
    bool     _emitting = false;            // non-blocking emission in flight
};


#include "RfRemoteStream.hpp"
#if defined(VH_RF_CC1101)
#include "RfRemoteCc1101.hpp"   // real OOK emitter (needs the CC1101 wired)
#endif

#endif
