#ifndef subfile_H_
#define subfile_H_

#include <Arduino.h>

/**
 * Flipper Zero .sub (SubGHz RAW) parser — pure, dependency-light, allocation-free.
 * See doc/SUBGHZ-FORMAT.md. Decodes the header (frequency + modulation) and the
 * RAW_Data timing stream (signed microseconds: + = carrier ON, - = OFF) into a
 * caller-provided buffer. No Arduino/U8g2 coupling -> unit-tested natively
 * (test/test_sub.cpp) on the CI, exactly like _can.h / _hudmath.h.
 *
 * This is the DATA layer of the OPTIONAL rf-replay add-on module. It parses;
 * it does NOT transmit. "Fixed code vs rolling code" is a property of the remote,
 * not of the file (see doc) — the parser stays neutral to it.
 *
 * Operates on a [begin, end) byte range (no NUL needed, no per-line copy), so an
 * arbitrarily long RAW_Data line streams through without a scratch buffer.
 */

// --- modulation, derived from the `Preset:` line ----------------------------
#define SUBFILE_MOD_UNKNOWN   0
#define SUBFILE_MOD_OOK       1   // FuriHalSubGhzPresetOok270Async / Ook650Async
#define SUBFILE_MOD_FSK       2   // FuriHalSubGhzPreset2FSKDev238Async / Dev476Async

// Timing element: signed microseconds. int16 (±32767) holds the symbols and the
// ~11-13 ms inter-frame gaps; the rare larger values (pre-press noise, very long
// gaps) saturate and raise SubFile.clamped. Override to int32_t for a capture
// that needs the full range (RAM is ample on the RP2040 "comfortable" tier).
#ifndef SUBFILE_DURATION_TYPE
#define SUBFILE_DURATION_TYPE int16_t
#endif
typedef SUBFILE_DURATION_TYPE SubfileDuration;

typedef struct {
    bool          validMagic;    // "Filetype: Flipper SubGhz RAW File" seen
    bool          isRaw;         // "Protocol: RAW" seen
    unsigned long frequencyHz;   // "Frequency:" value (Hz), 0 if absent
    uint8_t       modulation;    // SUBFILE_MOD_*
    size_t        count;         // timings written to the out buffer
    bool          truncated;     // out buffer full -> some timings dropped
    bool          clamped;       // a |duration| exceeded the element type -> saturated
} SubFile;


// ---------------------------------------------------------------------------
// small pure helpers (range-based; cstring shim only)
// ---------------------------------------------------------------------------

// If [b,e) begins with the NUL-terminated `key`, return the pointer just past
// the key and any run of spaces/tabs (i.e. the start of the value); else NULL.
static inline const char* subfile_matchKey(const char* b, const char* e, const char* key)
{
    const char* p = b;
    while (*key) {
        if (p >= e || *p != *key) { return 0; }
        ++p; ++key;
    }
    while (p < e && (*p == ' ' || *p == '\t')) { ++p; }
    return p;
}

// Does [b,e) contain the NUL-terminated `needle`? (tiny, for Preset class
// detection — needle is a handful of chars, so the naive scan is fine.)
static inline bool subfile_rangeContains(const char* b, const char* e, const char* needle)
{
    for (const char* p = b; p < e; ++p) {
        const char* a = p;
        const char* n = needle;
        while (*n && a < e && *a == *n) { ++a; ++n; }
        if (*n == '\0') { return true; }
    }
    return false;
}

// Parse one signed decimal integer in [*pp, e), skipping leading blanks.
// On success advances *pp past the digits, writes *val, returns true.
static inline bool subfile_nextInt(const char** pp, const char* e, long* val)
{
    const char* p = *pp;
    while (p < e && (*p == ' ' || *p == '\t')) { ++p; }
    bool neg = false;
    if (p < e && (*p == '-' || *p == '+')) { neg = (*p == '-'); ++p; }
    if (p >= e || *p < '0' || *p > '9') { *pp = p; return false; }
    long v = 0;
    while (p < e && *p >= '0' && *p <= '9') { v = v * 10 + (long)(*p - '0'); ++p; }
    *val = neg ? -v : v;
    *pp = p;
    return true;
}

// Append one duration (saturating to the element type's range) into out[].
static inline void subfile_append(SubFile* f, SubfileDuration* out, size_t cap, long v)
{
    const bool wide = (sizeof(SubfileDuration) >= 4);
    const long hi =  wide ?  2147483647L : 32767L;
    const long lo =  wide ? (-2147483647L - 1L) : -32768L;
    if (v > hi) { v = hi; f->clamped = true; }
    else if (v < lo) { v = lo; f->clamped = true; }
    if (f->count < cap) { out[f->count++] = (SubfileDuration)v; }
    else { f->truncated = true; }
}

// Dispatch a single line range [b,e) (trailing '\r' already stripped).
static inline void subfile_feedLine(const char* b, const char* e, SubFile* f,
                                    SubfileDuration* out, size_t cap)
{
    const char* v;

    if ((v = subfile_matchKey(b, e, "RAW_Data:")) != 0) {
        long n;
        const char* p = v;
        while (subfile_nextInt(&p, e, &n)) { subfile_append(f, out, cap, n); }
        return;
    }
    if ((v = subfile_matchKey(b, e, "Frequency:")) != 0) {
        long n;
        const char* p = v;
        if (subfile_nextInt(&p, e, &n) && n >= 0) { f->frequencyHz = (unsigned long)n; }
        return;
    }
    if ((v = subfile_matchKey(b, e, "Preset:")) != 0) {
        if (subfile_rangeContains(v, e, "Ook"))      { f->modulation = SUBFILE_MOD_OOK; }
        else if (subfile_rangeContains(v, e, "FSK")) { f->modulation = SUBFILE_MOD_FSK; }
        else                                         { f->modulation = SUBFILE_MOD_UNKNOWN; }
        return;
    }
    if ((v = subfile_matchKey(b, e, "Protocol:")) != 0) {
        // value is "RAW" (allow trailing blanks)
        f->isRaw = (v + 3 <= e) && (v[0] == 'R' && v[1] == 'A' && v[2] == 'W');
        return;
    }
    if ((v = subfile_matchKey(b, e, "Filetype:")) != 0) {
        f->validMagic = subfile_rangeContains(v, e, "Flipper SubGhz RAW File");
        return;
    }
    // any other line (Version:, comments, blanks) is ignored
}


// ---------------------------------------------------------------------------
// public entry point
// ---------------------------------------------------------------------------

/**
 * Parse `len` bytes of .sub text into `out` (cap elements). Returns a SubFile
 * with the header fields, the timing count, and the truncated/clamped flags.
 * Lines are split on '\n'; a trailing '\r' (CRLF files) is stripped. '#' is NOT
 * a Flipper comment marker — only our own sample uses it; such lines simply fail
 * every key match and are ignored.
 */
static inline SubFile subfile_parse(const char* text, size_t len,
                                    SubfileDuration* out, size_t cap)
{
    SubFile f;
    f.validMagic = false;
    f.isRaw = false;
    f.frequencyHz = 0;
    f.modulation = SUBFILE_MOD_UNKNOWN;
    f.count = 0;
    f.truncated = false;
    f.clamped = false;

    const char* p = text;
    const char* end = text + len;
    while (p < end) {
        const char* nl = p;
        while (nl < end && *nl != '\n') { ++nl; }
        const char* lineEnd = nl;
        if (lineEnd > p && *(lineEnd - 1) == '\r') { --lineEnd; }
        subfile_feedLine(p, lineEnd, &f, out, cap);
        p = (nl < end) ? (nl + 1) : end;
    }
    return f;
}


#endif
