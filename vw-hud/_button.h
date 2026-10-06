#ifndef button_H_
#define button_H_

#include <Arduino.h>

/**
 * Pure, dependency-light push-button debouncer — integrator + hysteresis
 * (software Schmitt trigger) + long-press timing. No Arduino coupling, so it is
 * unit-tested natively (test/test_button.cpp) like _can.h / _hudmath.h. The thin
 * Arduino glue (pinMode / digitalRead / millis) lives in RfRemote.
 *
 * WHY an integrator and not a "wait 20 ms then re-read":
 *   In a car, a deported button on a long wire sees EMI (ignition, injectors,
 *   alternator), contact bounce, AND a logic level that can hover near the
 *   VIH/VIL threshold when VCC droops or GND shifts. A single re-read still
 *   trusts one sample. The integrator instead COUNTS evidence: +1 each sample
 *   read as pressed (clamped at `max`), -1 each sample read as released (clamped
 *   at 0). The debounced state only flips at the rails (0 or max) and HOLDS in
 *   between — hysteresis. A lone glitch bumps the count by 1 and is forgotten;
 *   the state never chatters. (Classic Kenneth Kuhn integrator debounce.)
 *
 * WIRING this expects (documented in _wiring.h): ACTIVE-LOW with the internal
 * pull-up — button to GND. GND is the stable reference; a press is a hard pull
 * to 0 V, immune to VCC droop on a long run. The caller maps digitalRead()==LOW
 * to `rawPressed = true` before calling button_update().
 */

typedef struct {
    uint8_t  integrator;   // 0..max evidence counter
    bool     state;        // debounced logical state (true = pressed)
    bool     prevState;    // state at the previous update (for edges)
    uint32_t heldMs;       // ms continuously held pressed (0 when released)
    uint32_t prevHeldMs;   // heldMs at the previous update (for long-press edge)
} Button;


// Reset a button to the released/idle state.
static inline void button_init(Button* b)
{
    b->integrator = 0;
    b->state = false;
    b->prevState = false;
    b->heldMs = 0;
    b->prevHeldMs = 0;
}

/**
 * Feed one sample. `rawPressed` = the (already active-low-normalised) raw read,
 * `dtMs` = ms since the previous call (for long-press timing), `maxIntegrator` =
 * integration depth in samples (hysteresis width; 1 disables filtering).
 *
 * Call it at a roughly regular cadence. At ~16-32 ms/sample a depth of 3-4 gives
 * ~50-130 ms of debounce — comfortably past contact bounce without feeling laggy.
 */
static inline void button_update(Button* b, const bool rawPressed,
                                 const uint16_t dtMs, uint8_t maxIntegrator)
{
    if (maxIntegrator == 0) { maxIntegrator = 1; }

    // integrate evidence, clamped to [0, maxIntegrator]
    if (rawPressed) {
        if (b->integrator < maxIntegrator) { ++b->integrator; }
    } else {
        if (b->integrator > 0) { --b->integrator; }
    }

    // flip the debounced state only at the rails; hold it in between (hysteresis)
    b->prevState = b->state;
    if (b->integrator == 0) { b->state = false; }
    else if (b->integrator >= maxIntegrator) { b->state = true; }

    // accumulate hold time for long-press detection
    b->prevHeldMs = b->heldMs;
    b->heldMs = b->state ? (b->heldMs + dtMs) : 0;
}

// --- edge / level queries (evaluate after button_update) --------------------

static inline bool button_isPressed(const Button* b) { return b->state; }

// true for exactly one update when the button goes released -> pressed
static inline bool button_pressEdge(const Button* b) { return b->state && !b->prevState; }

// true for exactly one update when the button goes pressed -> released
static inline bool button_releaseEdge(const Button* b) { return !b->state && b->prevState; }

// true for exactly one update: the tick the hold time crosses `thresholdMs`
// (fires once per press; re-arms on release). Use for the "play" confirm.
static inline bool button_longPressEdge(const Button* b, const uint32_t thresholdMs)
{
    return (b->prevHeldMs < thresholdMs) && (b->heldMs >= thresholdMs);
}


#endif
