// Native unit tests for the pure button debouncer (_button.h). Build:
//   c++ -std=c++11 -Wall -Itest test/test_button.cpp -o test_button && ./test_button
//
// Focus: the LIMITS that matter in a noisy car — bounce rejection, single-glitch
// rejection near the logic threshold (hysteresis), clean edges, and long-press
// fire-once timing. These are exactly the "variations de tension" failure modes.
#include "../vw-hud/_button.h"
#include <cstdio>

static int failures = 0;

#define CHECK(name, got, want) do {                                  \
    const long long _g = (long long)(got);                           \
    const long long _w = (long long)(want);                          \
    if (_g != _w) { printf("FAIL %-34s got %lld want %lld\n", name, _g, _w); ++failures; } \
    else          { printf("ok   %-34s\n", name); }                  \
} while (0)

// feed the same raw level for n samples (dt ms each); return press-edge count
static int feed(Button* b, bool raw, int n, uint16_t dt, uint8_t depth)
{
    int edges = 0;
    for (int i = 0; i < n; ++i) {
        button_update(b, raw, dt, depth);
        if (button_pressEdge(b)) { ++edges; }
    }
    return edges;
}

int main()
{
    const uint8_t DEPTH = 4;    // integrator depth
    const uint16_t DT = 20;     // 20 ms/sample

    // ---- clean press: needs `depth` consistent samples to assert -------------
    {
        Button b; button_init(&b);
        button_update(&b, true, DT, DEPTH);  // integrator 1
        CHECK("press after 1 sample",  button_isPressed(&b), 0);   // not yet
        button_update(&b, true, DT, DEPTH);  // 2
        button_update(&b, true, DT, DEPTH);  // 3
        CHECK("press after 3 samples", button_isPressed(&b), 0);   // still rising
        button_update(&b, true, DT, DEPTH);  // 4 == depth -> flip
        CHECK("press at depth",        button_isPressed(&b), 1);
        CHECK("press edge fired",      button_pressEdge(&b), 1);
        button_update(&b, true, DT, DEPTH);  // stays pressed
        CHECK("edge is one-shot",      button_pressEdge(&b), 0);
    }

    // ---- single glitch near threshold must NOT register a press --------------
    // (one stray "pressed" sample among releases: integrator 0->1->0, never max)
    {
        Button b; button_init(&b);
        int edges = 0;
        bool pattern[] = {false,true,false,false,true,false,false,false,true,false};
        for (bool raw : pattern) {
            button_update(&b, raw, DT, DEPTH);
            if (button_pressEdge(&b)) ++edges;
        }
        CHECK("glitches -> no press",  button_isPressed(&b), 0);
        CHECK("glitches -> no edge",   edges, 0);
    }

    // ---- bounce on press: chatter then settle -> exactly one press edge ------
    {
        Button b; button_init(&b);
        int edges = 0;
        // noisy rising contact, then solidly held
        bool pattern[] = {true,false,true,false,true,true, true,true,true,true,true,true};
        for (bool raw : pattern) {
            button_update(&b, raw, DT, DEPTH);
            if (button_pressEdge(&b)) ++edges;
        }
        CHECK("bounce settles pressed", button_isPressed(&b), 1);
        CHECK("bounce -> one edge",     edges, 1);
    }

    // ---- release edge fires once, after `depth` release samples --------------
    {
        Button b; button_init(&b);
        feed(&b, true, DEPTH, DT, DEPTH);               // pressed
        CHECK("held before release",   button_isPressed(&b), 1);
        for (int i = 0; i < DEPTH - 1; ++i) button_update(&b, false, DT, DEPTH);
        CHECK("still held mid-release", button_isPressed(&b), 1);   // hysteresis
        button_update(&b, false, DT, DEPTH);            // integrator hits 0
        CHECK("released at rail",      button_isPressed(&b), 0);
        CHECK("release edge fired",    button_releaseEdge(&b), 1);
    }

    // ---- long press: fires once when hold time crosses the threshold ---------
    {
        Button b; button_init(&b);
        const uint32_t LONG = 500;    // ms
        int longs = 0;
        feed(&b, true, DEPTH, DT, DEPTH);               // assert press (4*20=80ms held)
        // keep holding; count long-press edges
        for (int i = 0; i < 40; ++i) {                  // +40*20 = 800 ms more
            button_update(&b, true, DT, DEPTH);
            if (button_longPressEdge(&b, LONG)) ++longs;
        }
        CHECK("long-press fires once", longs, 1);
        CHECK("still held after long", button_isPressed(&b), 1);
    }

    // ---- short press (released before threshold) -> NO long-press -------------
    {
        Button b; button_init(&b);
        const uint32_t LONG = 500;
        int longs = 0;
        // held ~ 4*20 + 5*20 = 180 ms total, then released
        feed(&b, true, DEPTH, DT, DEPTH);
        for (int i = 0; i < 5; ++i) { button_update(&b, true, DT, DEPTH); if (button_longPressEdge(&b, LONG)) ++longs; }
        feed(&b, false, DEPTH, DT, DEPTH);
        CHECK("short press no long",   longs, 0);
        CHECK("short press released",  button_isPressed(&b), 0);
    }

    // ---- long-press re-arms after release ------------------------------------
    {
        Button b; button_init(&b);
        const uint32_t LONG = 100;    // low threshold for the test
        int longs = 0;
        // press 1: hold past threshold
        feed(&b, true, DEPTH, DT, DEPTH);
        for (int i = 0; i < 6; ++i) { button_update(&b, true, DT, DEPTH); if (button_longPressEdge(&b, LONG)) ++longs; }
        // release
        feed(&b, false, DEPTH, DT, DEPTH);
        // press 2: hold past threshold again
        for (int i = 0; i < DEPTH; ++i) { button_update(&b, true, DT, DEPTH); if (button_longPressEdge(&b, LONG)) ++longs; }
        for (int i = 0; i < 6; ++i) { button_update(&b, true, DT, DEPTH); if (button_longPressEdge(&b, LONG)) ++longs; }
        CHECK("long-press re-arms",    longs, 2);
    }

    // ---- depth==0 guard (treated as 1: every sample flips immediately) -------
    {
        Button b; button_init(&b);
        button_update(&b, true, DT, 0);
        CHECK("depth0 immediate",      button_isPressed(&b), 1);
    }

    printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
