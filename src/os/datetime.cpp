#include "os/datetime.h"

#include <libcdvd.h>
#include <libscf.h>
#include <stdio.h>

#include "os/hxstr.h"

namespace {

// Two digits and a terminator fit with room to spare. The binary formats into 16 bytes of stack.
constexpr int kBcdTextSize = 16;

constexpr int kBcdDigitShift = 4;
constexpr unsigned char kBcdDigitMask = 0x0f;

// 0x0053a4d8
// The one out-of-line copy has no caller. FormatCurrentDateTime() inlines every use.
inline void FormatBcdByte(unsigned char nBcd, HxStr &text, bool bAppend) {
    char szDigits[kBcdTextSize];
    sprintf(szDigits, "%d%d", nBcd >> kBcdDigitShift, nBcd & kBcdDigitMask);
    if (bAppend) {
        text += szDigits;
    } else {
        text = szDigits;
    }
}

} // namespace

// 0x0053a108
bool FormatCurrentDateTime(HxStr &text) {
    sceCdCLOCK clock;
    if (sceCdReadClock(&clock) == 0) {
        return false;
    }
    sceScfGetLocalTimefromRTC(&clock);
    if (clock.stat != 0) {
        return false;
    }

    FormatBcdByte(clock.month, text, false);
    text += "/";
    FormatBcdByte(clock.day, text, true);
    text += "/";
    FormatBcdByte(clock.year, text, true);
    text += ", ";
    FormatBcdByte(clock.hour, text, true);
    text += ":";
    FormatBcdByte(clock.minute, text, true);
    return true;
}
