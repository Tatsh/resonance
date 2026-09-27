#include <libcdvd.h>
#include <libscf.h>

#include "os/assert.h"

enum {
    // Minutes in one hour.
    kMinutesPerHour = 60,
    // Minutes in nine hours of Tokyo time, the baseline the RTC keeps.
    kTokyoMinutes = 540,
    // The line the binary reports for a null clock argument.
    kNullClockLine = 455,
};

// 0x005f2ee8
extern int sceScfGetTimezone(void);

// 0x005f2fd0
extern int sceScfGetSummerTime(void);

// 0x005f3538
extern void sceScfApplyMinuteOffset(sceCdCLOCK *pClock, int nMinutes);

// 0x005f3650
void sceScfGetLocalTimefromRTC(sceCdCLOCK *pClock) {
    int nTimezone = sceScfGetTimezone();
    int nSummer = sceScfGetSummerTime();
    int nOffset = nTimezone + nSummer * kMinutesPerHour - kTokyoMinutes;

    if (pClock == NULL) {
        // The binary records the file as libscf.c with the line above.
        HxAssertFailed("libscf.c", kNullClockLine, "pClock != NULL");
    }
    sceScfApplyMinuteOffset(pClock, nOffset);
}
