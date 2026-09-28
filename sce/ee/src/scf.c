#include <assert.h>
#include <stdio.h>

#include <eekernel.h>
#include <libcdvd.h>
#include <libscf.h>
#include <sifdev.h>

enum {
    // Minutes in one hour.
    kMinutesPerHour = 60,
    // Minutes in nine hours of Tokyo time, the baseline the RTC keeps.
    kTokyoMinutes = 540,
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

// The ROM region letter of a tool console. A tool console reports the defaults below rather than
// the console configuration.
enum {
    kRomRegionIndex = 4,
    kRomRegionTool = 'T',
    kRomVersionReadSize = 14,
};

// The timezone a tool console reports, in minutes east of UTC.
// 0x0077fbf0
static short g_nScfDefaultTimezone = 540;

// The summer time flag a tool console reports.
// 0x0077fbf6
static unsigned char g_nScfDefaultSummerTime = 0;

// The contents of rom0:ROMVER, empty until the first read.
// 0x0077fbf8
static char g_szScfRomVersion[16];

// Helpers defined below for the minute offset path.
char *sceScfReadRomVersion(void);
void sceScfSub005f3190(sceCdCLOCK *pClock);

// Month lengths for the day arithmetic below, read from the image.
static const unsigned char kMonthLengths[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
};

// 0x005f2da8
// Reports whether the console runs a tool ROM, reading the ROM version first if needed.
int sceScfEnsureRomVersionRead(void) {
    if (g_szScfRomVersion[0] == '\0') {
        sceScfReadRomVersion();
    }
    return g_szScfRomVersion[kRomRegionIndex] == kRomRegionTool;
}

// 0x005f2d08
char *sceScfReadRomVersion(void) {
    int fd;

    if (g_szScfRomVersion[0] != '\0') {
        return g_szScfRomVersion;
    }
    fd = sceOpen("rom0:ROMVER", SCE_RDONLY);
    if (fd == -1) {
        printf("Can't open rom0:ROMVER\n");
    }
    // The binary reads even when the open failed.
    if (sceRead(fd, g_szScfRomVersion, kRomVersionReadSize) == -1) {
        printf("Can't read rom error\n");
    }
    sceClose(fd);
    return g_szScfRomVersion;
}

// 0x005f3138
int sceScfCheckBcdByte(int nValue) {
    unsigned int value = (unsigned int)nValue & 0xffu;

    assert(value <= 0x99u);
    return (int)((value - ((value >> 4) * 6u)) & 0xffu);
}

// 0x005f30d0
int sceScfCheckBcdBelowHundred(int nValue) {
    unsigned int value = (unsigned int)nValue & 0xffu;

    assert(value <= 99u);
    // The image guards a divide by zero against the constant divisor, which cannot fire.
    return (int)((value / 10u) * 6u + value);
}

// 0x005f32a0
void sceScfSub005f32a0(sceCdCLOCK *pClock) {
    unsigned char monthLengths[12];
    int i;

    assert(pClock != NULL);
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

    assert(pClock != NULL);
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
    assert(pClock != NULL);
    pClock->year = (unsigned char)sceScfCheckBcdByte(pClock->year);
    pClock->month = (unsigned char)sceScfCheckBcdByte(pClock->month);
    pClock->day = (unsigned char)sceScfCheckBcdByte(pClock->day);
    pClock->hour = (unsigned char)sceScfCheckBcdByte(pClock->hour);
    pClock->minute = (unsigned char)sceScfCheckBcdByte(pClock->minute);
    pClock->second = (unsigned char)sceScfCheckBcdByte(pClock->second);
}

// 0x005f3218
void sceScfSub005f3218(sceCdCLOCK *pClock) {
    assert(pClock != NULL);
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

    assert(pClock != NULL);
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

    assert(pClock != NULL);
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
        return g_nScfDefaultTimezone;
    }
    GetOsdConfigParam(&nConfig);
    nVersion = (nConfig >> kVersionShift) & (unsigned int)kVersionMask;
    if (nVersion == 0u) {
        return kTokyoMinutes;
    }
    nTimezone = (int)nConfig >> kTimezoneShift;
    printf("Timezone=%d\n", nTimezone);
    return nTimezone;
}

// 0x005f2fd0
int sceScfGetSummerTime(void) {
    unsigned int nConfig;
    unsigned char nDetail;
    unsigned int nVersion;
    int nSummer;

    if (sceScfEnsureRomVersionRead() != 0) {
        return g_nScfDefaultSummerTime;
    }
    GetOsdConfigParam(&nConfig);
    nVersion = (nConfig >> kVersionShift) & (unsigned int)kVersionMask;
    if (nVersion == 0u) {
        return 0;
    }
    GetOsdConfigParam2(&nDetail, 1, 1);
    nSummer = (nDetail >> kDaylightShift) & 1;
    printf("SummerTime=%d\n", nSummer);
    return nSummer;
}

// 0x005f3538
void sceScfApplyMinuteOffset(sceCdCLOCK *pClock, int nMinutes) {
    int nTotal;

    assert(pClock != NULL);
    assert((unsigned int)(nMinutes + kHalfDayMinutes) < (unsigned int)kOffsetRangeWidth);
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

    assert(pClock != NULL);
    sceScfApplyMinuteOffset(pClock, nOffset);
}
