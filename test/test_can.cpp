// Native unit tests for the CAN decode helper. Build:
//   c++ -std=c++11 -Wall -Itest test/test_can.cpp -o test_can && ./test_can
#include "../vw-hud/_can.h"
#include <cstdio>

static int failures = 0;

#define CHECK(name, got, want) do {                                  \
    const unsigned long _g = (unsigned long)(got);                   \
    const unsigned long _w = (unsigned long)(want);                  \
    if (_g != _w) { printf("FAIL %-22s got %lu want %lu\n", name, _g, _w); ++failures; } \
    else          { printf("ok   %-22s\n", name); }                  \
} while (0)

// write `value` little-endian into `buf` at bit offset `start`, `len` bits
static void putLE(unsigned char* buf, uint8_t start, uint8_t len, unsigned long value)
{
    for (uint8_t i = 0; i < len; ++i) {
        if (value & (1UL << i)) {
            const uint8_t bit = start + i;
            buf[bit >> 3] |= (unsigned char)(1u << (bit & 7));
        }
    }
}

int main()
{
    unsigned char b[8];

    memset(b, 0, 8); b[0] = 0x12;
    CHECK("byte0 8b@0", canExtractLE(b, 0, 8), 0x12);

    memset(b, 0, 8); b[2] = 0x34; b[3] = 0x12;
    CHECK("u16le@16", canExtractLE(b, 16, 16), 0x1234);

    // RPM raw = rpm*4 ; rpm 2000 -> 8000 = 0x1F40 (Motor_1 bit16 len16)
    memset(b, 0, 8); b[2] = 0x40; b[3] = 0x1F;
    CHECK("rpm raw 8000", canExtractLE(b, VH_SIG_RPM_START, VH_SIG_RPM_LEN), 8000);

    // 1-bit fields (ignition / handbrake at bit 1)
    memset(b, 0, 8); b[0] = 0x02;
    CHECK("bit1 set", canExtractLE(b, 1, 1), 1);
    memset(b, 0, 8); b[0] = 0x01;
    CHECK("bit1 clear", canExtractLE(b, 1, 1), 0);

    // 7-bit tank must ignore the 8th bit
    memset(b, 0, 8); b[2] = (unsigned char)(0x80 | 55);
    CHECK("tank 7b masks bit23", canExtractLE(b, VH_SIG_TANK_START, VH_SIG_TANK_LEN), 55);

    // cross-byte 15-bit speed @25 (round-trip)
    memset(b, 0, 8); putLE(b, VH_SIG_SPEED_START, VH_SIG_SPEED_LEN, 1234);
    CHECK("speed 15b@25", canExtractLE(b, VH_SIG_SPEED_START, VH_SIG_SPEED_LEN), 1234);

    // 20-bit odometer @40 (round-trip)
    memset(b, 0, 8); putLE(b, VH_SIG_ODO_START, VH_SIG_ODO_LEN, 142537);
    CHECK("odo 20b@40", canExtractLE(b, VH_SIG_ODO_START, VH_SIG_ODO_LEN), 142537);

    // 15-bit consumption counter near its max (0x7FFF)
    memset(b, 0, 8); putLE(b, VH_SIG_CONS_START, VH_SIG_CONS_LEN, 0x7FFF);
    CHECK("cons 15b max", canExtractLE(b, VH_SIG_CONS_START, VH_SIG_CONS_LEN), 0x7FFF);

    // top bit of the frame
    memset(b, 0, 8); b[7] = 0x80;
    CHECK("bit63", canExtractLE(b, 63, 1), 1);

    printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
