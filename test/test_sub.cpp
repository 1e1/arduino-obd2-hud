// Native unit tests for the pure Flipper .sub parser (_subfile.h). Build:
//   c++ -std=c++11 -Wall -Itest test/test_sub.cpp -o test_sub && ./test_sub
//
// Focus: header field extraction (freq / modulation / magic / RAW), RAW_Data
// tokenizing (signs, multi-line concat), and the LIMIT flags (saturation on
// out-of-range durations, truncation when the buffer is full).
#include "../vw-hud/_subfile.h"
#include <cstdio>

static int failures = 0;

#define CHECK(name, got, want) do {                                  \
    const long long _g = (long long)(got);                           \
    const long long _w = (long long)(want);                          \
    if (_g != _w) { printf("FAIL %-32s got %lld want %lld\n", name, _g, _w); ++failures; } \
    else          { printf("ok   %-32s\n", name); }                  \
} while (0)

template <size_t N>
static SubFile parse(const char (&text)[N], SubfileDuration* out, size_t cap)
{
    // N-1: drop the implicit trailing NUL of the string literal
    return subfile_parse(text, N - 1, out, cap);
}

int main()
{
    SubfileDuration buf[64];

    // ---- a full, well-formed OOK/RAW capture (two RAW_Data lines) ------------
    {
        const char sub[] =
            "Filetype: Flipper SubGhz RAW File\n"
            "Version: 1\n"
            "Frequency: 433920000\n"
            "Preset: FuriHalSubGhzPresetOok650Async\n"
            "Protocol: RAW\n"
            "RAW_Data: 350 -350 1050 -350 350 -1050\n"
            "RAW_Data: 1050 -350 350 -10850\n";
        SubFile f = parse(sub, buf, 64);
        CHECK("magic",            f.validMagic, 1);
        CHECK("isRaw",            f.isRaw, 1);
        CHECK("frequency Hz",     f.frequencyHz, 433920000UL);
        CHECK("modulation OOK",   f.modulation, SUBFILE_MOD_OOK);
        CHECK("not truncated",    f.truncated, 0);
        CHECK("not clamped",      f.clamped, 0);
        // 6 values on line 1 + 4 on line 2 = 10, concatenated across lines
        CHECK("count concat",     f.count, 10);
        CHECK("first ON",         buf[0], 350);
        CHECK("first OFF sign",   buf[1], -350);
        CHECK("cross-line [6]",   buf[6], 1050);
        CHECK("last gap",         buf[9], -10850);
    }

    // ---- modulation: 2-FSK preset is detected (and flagged, not OOK) ---------
    {
        const char sub[] =
            "Filetype: Flipper SubGhz RAW File\n"
            "Preset: FuriHalSubGhzPreset2FSKDev476Async\n"
            "Protocol: RAW\n"
            "RAW_Data: 100 -100\n";
        SubFile f = parse(sub, buf, 64);
        CHECK("modulation FSK",   f.modulation, SUBFILE_MOD_FSK);
        CHECK("fsk count",        f.count, 2);
    }

    // ---- a non-RAW protocol: header parses, no timings ingested --------------
    {
        const char sub[] =
            "Filetype: Flipper SubGhz Key File\n"
            "Frequency: 868350000\n"
            "Preset: FuriHalSubGhzPresetOok650Async\n"
            "Protocol: KeeLoq\n"
            "Bit: 64\n"
            "Key: 00 11 22 33 44 55 66 77\n";
        SubFile f = parse(sub, buf, 64);
        CHECK("keeloq not raw",   f.isRaw, 0);
        CHECK("keeloq magic off", f.validMagic, 0);   // "Key File" != "RAW File"
        CHECK("keeloq freq",      f.frequencyHz, 868350000UL);
        CHECK("keeloq no timings", f.count, 0);
    }

    // ---- saturation: a pre-press noise value exceeds int16, must clamp -------
    {
        const char sub[] =
            "Protocol: RAW\n"
            "RAW_Data: 146991 -13950 250 -250\n";   // 146991 > 32767, -13950 fits
        SubFile f = parse(sub, buf, 64);
        CHECK("sat count",        f.count, 4);
        CHECK("sat clamped flag", f.clamped, 1);
        CHECK("sat value hi",     buf[0], 32767);    // saturated, not wrapped
        CHECK("sat neg kept",     buf[1], -13950);   // in range, untouched
        CHECK("sat normal",       buf[2], 250);
    }

    // ---- truncation: buffer smaller than the timing stream -------------------
    {
        const char sub[] =
            "Protocol: RAW\n"
            "RAW_Data: 10 -10 20 -20 30 -30 40 -40\n";  // 8 values
        SubfileDuration small[4];
        SubFile f = subfile_parse(sub, sizeof(sub) - 1, small, 4);
        CHECK("trunc count cap",  f.count, 4);          // stopped at cap
        CHECK("trunc flag",       f.truncated, 1);
        CHECK("trunc kept[3]",    small[3], -20);       // 4th value is -20
    }

    // ---- CRLF line endings are tolerated (trailing '\r' stripped) ------------
    {
        const char sub[] =
            "Frequency: 315000000\r\n"
            "Protocol: RAW\r\n"
            "RAW_Data: 500 -500\r\n";
        SubFile f = parse(sub, buf, 64);
        CHECK("crlf freq",        f.frequencyHz, 315000000UL);
        CHECK("crlf isRaw",       f.isRaw, 1);          // 'RAW\r' -> value still RAW
        CHECK("crlf count",       f.count, 2);
        CHECK("crlf gap kept",    buf[1], -500);
    }

    // ---- our synthetic fixture's first word (doc/subghz-sample.sub) ----------
    // bit '1' = 1050 -350 ; bit '0' = 350 -1050 ; 0xA5C3F0 = 1010 0101 ...
    {
        const char sub[] =
            "Filetype: Flipper SubGhz RAW File\n"
            "Frequency: 433920000\n"
            "Preset: FuriHalSubGhzPresetOok650Async\n"
            "Protocol: RAW\n"
            "RAW_Data: 1050 -350 350 -1050 1050 -350 350 -1050\n"; // 1 0 1 0
        SubFile f = parse(sub, buf, 64);
        CHECK("fixture count",    f.count, 8);
        CHECK("fixture bit1 ON",  buf[0], 1050);   // '1'
        CHECK("fixture bit1 OFF", buf[1], -350);
        CHECK("fixture bit0 ON",  buf[2], 350);    // '0'
        CHECK("fixture bit0 OFF", buf[3], -1050);
    }

    printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
