#include <eekernel.h>
#include <libcdvd.h>
#include <libscf.h>

#include "os/assert.h"
#include "os/log.h"

enum {
    // Minutes in one hour.
    kMinutesPerHour = 60,
    // Minutes in nine hours of Tokyo time, the baseline the RTC keeps.
    kTokyoMinutes = 540,
    // The line the binary reports for a null clock argument.
    kNullClockLine = 455,
    // The offset routine reports this line for a null clock.
    kOffsetNullLine = 416,
    // The offset routine reports this line for an out of range offset.
    kOffsetRangeLine = 417,
    // Half the minutes in one day define the largest accepted offset magnitude.
    kHalfDayMinutes = 1440,
    // Twice the largest magnitude plus one gives the accepted range width.
    kOffsetRangeWidth = 2881,
    // Minutes in one hour set the threshold for a carry into the hour field.
    kHourThreshold = 60,
    // The version field starts after this many bits in the configuration word.
    kVersionShift = 13,
    // The version field uses this mask after the shift.
    kVersionMask = 7,
    // The timezone field starts after this many bits in the configuration word.
    kTimezoneShift = 21,
    // The daylight flag starts after this many bits in the detail byte.
    kDaylightShift = 4,
};

// 0x005f2da8
extern int sceScfEnsureRomVersionRead(void);

// 0x005f3190
extern void sceScfSub005f3190(sceCdCLOCK *pClock);

// 0x005f3218
extern void sceScfSub005f3218(sceCdCLOCK *pClock);

// 0x005f3460
extern void sceScfSub005f3460(sceCdCLOCK *pClock);

// 0x005f34d0
extern void sceScfSub005f34d0(sceCdCLOCK *pClock);

// 0x005f2ee8
int sceScfGetTimezone(void) {
    unsigned int nConfig;
    unsigned int nVersion;
    int nTimezone;

    if (sceScfEnsureRomVersionRead() != 0) {
        return (int)*(short *)0x77FBF0;
    }
    GetOsdConfigParam(&nConfig);
    nVersion = (nConfig >> kVersionShift) & (unsigned int)kVersionMask;
    if (nVersion == 0u) {
        return kTokyoMinutes;
    }
    nTimezone = (int)nConfig >> kTimezoneShift;
    LogPrintf("Timezone=%d\n", nTimezone);
    return nTimezone;
}

// 0x005f2fd0
int sceScfGetSummerTime(void) {
    unsigned int nConfig;
    unsigned char nDetail;
    unsigned int nVersion;
    int nSummer;

    if (sceScfEnsureRomVersionRead() != 0) {
        return *(unsigned char *)0x77FBF6;
    }
    GetOsdConfigParam(&nConfig);
    nVersion = (nConfig >> kVersionShift) & (unsigned int)kVersionMask;
    if (nVersion == 0u) {
        return 0;
    }
    GetOsdConfigParam2(&nDetail, 1, 1);
    nSummer = (nDetail >> kDaylightShift) & 1;
    LogPrintf("SummerTime=%d\n", nSummer);
    return nSummer;
}

// 0x005f3538
void sceScfApplyMinuteOffset(sceCdCLOCK *pClock, int nMinutes) {
    int nTotal;

    if (pClock == NULL) {
        HxAssertFailed("libscf.c", kOffsetNullLine, "prtc != NULL");
    }
    if ((unsigned int)(nMinutes + kHalfDayMinutes) >= (unsigned int)kOffsetRangeWidth) {
        HxAssertFailed("libscf.c", kOffsetRangeLine, "-60*24<=diff && diff <= 60*24");
    }
    sceScfSub005f3190(pClock);
    nTotal = pClock->minute + nMinutes;
    if (nTotal < 0) {
        do {
            nTotal += kHourThreshold;
            sceScfSub005f34d0(pClock);
        } while (nTotal < 0);
        pClock->minute = (unsigned char)nTotal;
    } else if (nTotal < kHourThreshold) {
        pClock->minute = (unsigned char)nTotal;
    } else {
        do {
            nTotal -= kHourThreshold;
            sceScfSub005f3460(pClock);
        } while (nTotal >= kHourThreshold);
        pClock->minute = (unsigned char)nTotal;
    }
    sceScfSub005f3218(pClock);
}

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
