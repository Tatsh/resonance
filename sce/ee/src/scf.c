#include <eekernel.h>
#include <libcdvd.h>
#include <libscf.h>
#include <stdint.h>

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

// Helpers defined below for the minute offset path.
int sceScfReadRomVersion(void);
void sceScfSub005f3190(sceCdCLOCK *pClock);

// Month lengths for the day arithmetic below, read from the image.
static const unsigned char kMonthLengths[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
};

// Helpers the read path below needs, provided by the file layer and the toolchain.
extern int sceFileioRpcRoutine0056b340(int nFile, void *pBuffer, int nSize);
extern int sceOpen(const char *pathname, int flags);
extern int sceClose(int nFile);

// 0x005f2da8
int sceScfEnsureRomVersionRead(void) {
    if (*(volatile signed char *)(uintptr_t)0x77fbf8 != 0) {
        return 0x77fbf8;
    }
    sceScfReadRomVersion();
    return *(volatile signed char *)(uintptr_t)0x77fbfc == 0x54 ? 1 : 0;
}

// 0x005f2d08
int sceScfReadRomVersion(void) {
    int fd;

    if (*(volatile char *)(uintptr_t)0x77fbf8 != 0) {
        return 0x77fbf8;
    }
    fd = sceOpen("rom0:ROMVER", 1);
    if (fd == -1) {
        LogPrintf("Can't open rom0:ROMVER\n");
    }
    // The buffer argument survives in the register from the report call. The reconstruction
    // passes the version area it points at in every observed run.
    if (sceFileioRpcRoutine0056b340(fd, (void *)(uintptr_t)0x77fbf8, 0xe) == -1) {
        LogPrintf("Can't read rom error\n");
    }
    sceClose(fd);
    return 0x77fbf8;
}

// 0x005f3138
int sceScfCheckBcdByte(int nValue) {
    unsigned int value = (unsigned int)nValue & 0xffu;

    if (value >= 0x9au) {
        HxAssertFailed("libscf.c", 0x126, "c <= 0x99");
    }
    return (int)((value - ((value >> 4) * 6u)) & 0xffu);
}

// 0x005f30d0
int sceScfCheckBcdBelowHundred(int nValue) {
    unsigned int value = (unsigned int)nValue & 0xffu;

    if (value >= 0x64u) {
        HxAssertFailed("libscf.c", 0x119, "c <=99");
    }
    // The image guards a divide by zero against the constant divisor, which cannot fire.
    return (int)((value / 10u) * 6u + value);
}

// 0x005f32a0
void sceScfSub005f32a0(sceCdCLOCK *pClock) {
    unsigned char monthLengths[12];
    int i;

    if (pClock == NULL) {
        HxAssertFailed("libscf.c", 0x150, "prtc != NULL");
    }
    for (i = 0; i < 12; ++i) {
        monthLengths[i] = kMonthLengths[i];
    }
    pClock->day++;
    if ((pClock->year & 3) == 0) {
        monthLengths[1] = 0x1d;
    }
    if (pClock->day <= monthLengths[pClock->month - 1]) {
        return;
    }
    pClock->day = 1;
    pClock->month++;
    if (pClock->month != 0xd) {
        return;
    }
    if (pClock->year != 0x63) {
        pClock->year++;
        return;
    }
    pClock->year = 0;
    pClock->month = 1;
}

// 0x005f3388
void sceScfSub005f3388(sceCdCLOCK *pClock) {
    unsigned char monthLengths[12];
    int i;

    if (pClock == NULL) {
        HxAssertFailed("libscf.c", 0x168, "prtc != NULL");
    }
    for (i = 0; i < 12; ++i) {
        monthLengths[i] = kMonthLengths[i];
    }
    pClock->day--;
    if ((pClock->year & 3) == 0) {
        monthLengths[1] = 0x1d;
    }
    if (pClock->day != 0) {
        return;
    }
    pClock->month--;
    if (pClock->month != 0) {
        pClock->day = monthLengths[pClock->month - 1];
        return;
    }
    if (pClock->year != 0) {
        pClock->year--;
    } else {
        pClock->year = 0x63;
    }
    pClock->month = 0xc;
    pClock->day = monthLengths[11];
}

// 0x005f3190
void sceScfSub005f3190(sceCdCLOCK *pClock) {
    if (pClock == NULL) {
        HxAssertFailed("libscf.c", 0x132, "prtc != NULL");
    }
    pClock->year = (unsigned char)sceScfCheckBcdByte(pClock->year);
    pClock->month = (unsigned char)sceScfCheckBcdByte(pClock->month);
    pClock->day = (unsigned char)sceScfCheckBcdByte(pClock->day);
    pClock->hour = (unsigned char)sceScfCheckBcdByte(pClock->hour);
    pClock->minute = (unsigned char)sceScfCheckBcdByte(pClock->minute);
    pClock->second = (unsigned char)sceScfCheckBcdByte(pClock->second);
}

// 0x005f3218
void sceScfSub005f3218(sceCdCLOCK *pClock) {
    if (pClock == NULL) {
        HxAssertFailed("libscf.c", 0x141, "prtc != NULL");
    }
    pClock->year = (unsigned char)sceScfCheckBcdBelowHundred(pClock->year);
    pClock->month = (unsigned char)sceScfCheckBcdBelowHundred(pClock->month);
    pClock->day = (unsigned char)sceScfCheckBcdBelowHundred(pClock->day);
    pClock->hour = (unsigned char)sceScfCheckBcdBelowHundred(pClock->hour);
    pClock->minute = (unsigned char)sceScfCheckBcdBelowHundred(pClock->minute);
    pClock->second = (unsigned char)sceScfCheckBcdBelowHundred(pClock->second);
}

// 0x005f3460
void sceScfSub005f3460(sceCdCLOCK *pClock) {
    unsigned int hour;

    if (pClock == NULL) {
        HxAssertFailed("libscf.c", 0x181, "prtc != NULL");
    }
    hour = (unsigned int)pClock->hour + 1u;
    if ((hour & 0xffu) != 0x18u) {
        pClock->hour = (unsigned char)hour;
        return;
    }
    pClock->hour = 0;
    sceScfSub005f32a0(pClock);
}

// 0x005f34d0
void sceScfSub005f34d0(sceCdCLOCK *pClock) {
    unsigned int hour;

    if (pClock == NULL) {
        HxAssertFailed("libscf.c", 0x18e, "prtc != NULL");
    }
    hour = pClock->hour;
    if (hour == 0) {
        pClock->hour = 0x17;
        sceScfSub005f3388(pClock);
        return;
    }
    pClock->hour = (unsigned char)(hour - 1u);
}

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
