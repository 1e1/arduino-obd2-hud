#ifndef RfLibrary_H_
#define RfLibrary_H_

#include <Arduino.h>
#include "_wiring.h"
#include "_subfile.h"

#if defined(ARDUINO_ARCH_RP2040)
#include <LittleFS.h>   // phase 4: .sub files live on the RP2040 flash FS
#endif

/**
 * Library of loaded .sub captures for the rf-replay add-on (phase 4 data layer).
 *
 * Holds METADATA for up to RF_MAX_PORTALS entries (display name + frequency),
 * and reads the TIMINGS lazily at replay time (one shared scratch), so RAM stays
 * bounded regardless of file size. Two sources:
 *   - LittleFS: loadFromFs() scans "/" for "*.sub" (RP2040).
 *   - In-memory: add() registers a .sub held in flash/RAM (e.g. a baked sample)
 *     without a filesystem — handy on the bench.
 *
 * names() returns the pointer array to hand straight to RfRemote::setNames().
 */

#ifndef RF_MAX_PORTALS
#define RF_MAX_PORTALS   6
#endif
#ifndef RF_NAME_MAX
#define RF_NAME_MAX      16    // display name (fits the OLED fuel band)
#endif
#ifndef RF_PATH_MAX
#define RF_PATH_MAX      48
#endif
#ifndef RF_SUB_TEXT_MAX
#define RF_SUB_TEXT_MAX  12288 // scratch to read one .sub file for timing parse
#endif
#ifndef RF_MAX_TIMINGS
#define RF_MAX_TIMINGS   4096  // max transitions per replay (int16 -> 8 KB buffer)
#endif

class RfLibrary {

  public:
    uint8_t     count(void) const            { return this->_count; }
    const char* name(const uint8_t i) const  { return (i < this->_count) ? this->_entry[i].name : ""; }
    unsigned long freqHz(const uint8_t i) const { return (i < this->_count) ? this->_entry[i].freqHz : 0UL; }
    const char* const* names(void) const     { return this->_names; }  // -> RfRemote::setNames()

    // Register an in-memory .sub (not copied; `text` must outlive this object).
    // Parses the header for the frequency/modulation; timings parsed on demand.
    bool add(const char* name, const char* text, const size_t len)
    {
        if (this->_count >= RF_MAX_PORTALS) { return false; }
        Entry& e = this->_entry[this->_count];
        this->_copy(e.name, name, RF_NAME_MAX);
        e.path[0] = '\0';
        e.text = text; e.textLen = len;
        const SubFile sf = subfile_parse(text, len, nullptr, 0);  // header only (cap 0)
        e.freqHz = sf.frequencyHz; e.modulation = sf.modulation;
        this->_names[this->_count] = e.name;
        ++this->_count;
        return true;
    }

    // Scan LittleFS for *.sub, fill name + frequency metadata. Returns count.
    uint8_t loadFromFs(void)
    {
        #if defined(ARDUINO_ARCH_RP2040)
        if (!LittleFS.begin()) { return this->_count; }
        File root = LittleFS.open("/", "r");
        if (!root) { return this->_count; }
        for (File f = root.openNextFile(); f; f = root.openNextFile()) {
            const char* fn = f.name();
            if (!f.isDirectory() && this->_isSub(fn) && this->_count < RF_MAX_PORTALS) {
                Entry& e = this->_entry[this->_count];
                this->_nameFromPath(e.name, fn);
                this->_copy(e.path, fn, RF_PATH_MAX);
                e.text = nullptr; e.textLen = 0;
                char hdr[192];
                const size_t n = f.readBytes(hdr, sizeof(hdr) - 1);
                const SubFile sf = subfile_parse(hdr, n, nullptr, 0);  // header fields
                e.freqHz = sf.frequencyHz; e.modulation = sf.modulation;
                this->_names[this->_count] = e.name;
                ++this->_count;
            }
            f.close();
        }
        #endif
        return this->_count;
    }

    // (Re)read entry i's timings into out[cap]. Returns the SubFile (count/flags).
    SubFile loadTimings(const uint8_t i, SubfileDuration* out, const size_t cap) const
    {
        SubFile empty; empty.validMagic = false; empty.isRaw = false; empty.frequencyHz = 0;
        empty.modulation = SUBFILE_MOD_UNKNOWN; empty.count = 0; empty.truncated = false; empty.clamped = false;
        if (i >= this->_count) { return empty; }
        const Entry& e = this->_entry[i];

        if (e.text != nullptr) {                    // in-memory source
            return subfile_parse(e.text, e.textLen, out, cap);
        }
        #if defined(ARDUINO_ARCH_RP2040)
        File f = LittleFS.open(e.path, "r");         // filesystem source
        if (f) {
            static char buf[RF_SUB_TEXT_MAX];        // single shared scratch (not on stack)
            const size_t n = f.readBytes(buf, sizeof(buf));
            f.close();
            return subfile_parse(buf, n, out, cap);
        }
        #endif
        return empty;
    }

  protected:
    struct Entry {
        char          name[RF_NAME_MAX];
        char          path[RF_PATH_MAX];
        unsigned long freqHz;
        uint8_t       modulation;
        const char*   text;       // non-null = in-memory source
        uint16_t      textLen;
    };

    static bool _isSub(const char* s)
    {
        size_t n = 0; while (s[n]) { ++n; }
        if (n < 4) { return false; }
        const char* e = s + n - 4;
        return (e[0] == '.' && (e[1] | 32) == 's' && (e[2] | 32) == 'u' && (e[3] | 32) == 'b');
    }

    static void _copy(char* dst, const char* src, const size_t cap)
    {
        size_t i = 0; for (; src[i] && i < cap - 1; ++i) { dst[i] = src[i]; } dst[i] = '\0';
    }

    // display name = filename without leading dir and without ".sub"
    static void _nameFromPath(char* dst, const char* path)
    {
        const char* base = path;
        for (const char* p = path; *p; ++p) { if (*p == '/') { base = p + 1; } }
        size_t i = 0; for (; base[i] && base[i] != '.' && i < RF_NAME_MAX - 1; ++i) { dst[i] = base[i]; }
        dst[i] = '\0';
    }

    Entry       _entry[RF_MAX_PORTALS];
    const char* _names[RF_MAX_PORTALS] = {0};
    uint8_t     _count = 0;
};


#endif
