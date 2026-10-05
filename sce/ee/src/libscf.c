#include <assert.h>
#include <stdio.h>

#include <eekernel.h>
#include <libcdvd.h>
#include <libscf.h>
#include <sifdev.h>

#include "os/log.h"

enum {
    // Minutes in one hour.
    kMinutesPerHour = 60,
    // Minutes in nine hours of Tokyo time, the baseline the RTC keeps.
    kTokyoMinutes = 540,
    // Half the minutes in one day define the largest accepted offset magnitude.
    kHalfDayMinutes = 1440,
    // Twice the largest magnitude plus one gives the accepted range width.
    kOffsetRangeWidth = 2881,
    // The lowest minute total that carries into the hour field. A total of exactly 60 stays in the
    // minute field, as in the binary.
    kHourThreshold = 61,
    // The version field starts after this many bits in the configuration word.
    kVersionShift = 13,
    // The version field uses this mask after the shift.
    kVersionMask = 7,
    // The language field starts after this many bits in the configuration word.
    kLanguageShift = 16,
    // The language field uses this mask after the shift.
    kLanguageMask = 0x1f,
    // A version 0 configuration word has a one-bit language at this position instead.
    kLegacyLanguageShift = 4,
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
// NTSC-U/C: 0x0077fbf0, PAL: 0x007a4678
static short g_nScfDefaultTimezone = 540;

// The language a tool console reports.
// NTSC-U/C: 0x0077fbf4, PAL: 0x007a467c
static unsigned char g_nScfDefaultLanguage = SCE_JAPANESE_LANGUAGE;

// The summer time flag a tool console reports.
// NTSC-U/C: 0x0077fbf6, PAL: 0x007a467e
static unsigned char g_nScfDefaultSummerTime = 0;

// The contents of rom0:ROMVER, empty until the first read.
// NTSC-U/C: 0x0077fbf8, PAL: 0x007a4680
static char g_szScfRomVersion[16];

// Month lengths for the day arithmetic below, read from the image.
static const unsigned char kMonthLengths[12] = {
    31,
    28,
    31,
    30,
    31,
    30,
    31,
    31,
    30,
    31,
    30,
    31,
};

// NTSC-U/C: 0x005f2d08, PAL: 0x005a5910
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

// NTSC-U/C: 0x005f2da8, PAL: 0x005a59b0
// Reports whether the console runs a tool ROM, reading the ROM version first if needed.
int sceScfEnsureRomVersionRead(void) {
    if (g_szScfRomVersion[0] == '\0') {
        sceScfReadRomVersion();
    }
    return g_szScfRomVersion[kRomRegionIndex] == kRomRegionTool;
}

// NTSC-U/C: 0x005f3138, PAL: 0x005a5d40
int sceScfBcdToBinary(int nValue) {
    unsigned int value = (unsigned int)nValue & 0xffu;

    assert(value <= 0x99u);
    return (int)((value - ((value >> 4) * 6u)) & 0xffu);
}

// NTSC-U/C: 0x005f30d0, PAL: 0x005a5cd8
int sceScfBinaryToBcd(int nValue) {
    unsigned int value = (unsigned int)nValue & 0xffu;

    assert(value <= 99u);
    // The image guards a divide by zero against the constant divisor, which cannot fire.
    return (int)((value / 10u) * 6u + value);
}

// NTSC-U/C: 0x005f32a0, PAL: 0x005a5ea8
void sceScfAdvanceDay(sceCdCLOCK *pClock) {
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

// NTSC-U/C: 0x005f3388, PAL: 0x005a5f90
void sceScfRewindDay(sceCdCLOCK *pClock) {
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

// NTSC-U/C: 0x005f3190, PAL: 0x005a5d98
void sceScfClockFromBcd(sceCdCLOCK *pClock) {
    assert(pClock != NULL);
    pClock->year = (unsigned char)sceScfBcdToBinary(pClock->year);
    pClock->month = (unsigned char)sceScfBcdToBinary(pClock->month);
    pClock->day = (unsigned char)sceScfBcdToBinary(pClock->day);
    pClock->hour = (unsigned char)sceScfBcdToBinary(pClock->hour);
    pClock->minute = (unsigned char)sceScfBcdToBinary(pClock->minute);
    pClock->second = (unsigned char)sceScfBcdToBinary(pClock->second);
}

// NTSC-U/C: 0x005f3218, PAL: 0x005a5e20
void sceScfClockToBcd(sceCdCLOCK *pClock) {
    assert(pClock != NULL);
    pClock->year = (unsigned char)sceScfBinaryToBcd(pClock->year);
    pClock->month = (unsigned char)sceScfBinaryToBcd(pClock->month);
    pClock->day = (unsigned char)sceScfBinaryToBcd(pClock->day);
    pClock->hour = (unsigned char)sceScfBinaryToBcd(pClock->hour);
    pClock->minute = (unsigned char)sceScfBinaryToBcd(pClock->minute);
    pClock->second = (unsigned char)sceScfBinaryToBcd(pClock->second);
}

// NTSC-U/C: 0x005f3460, PAL: 0x005a6068
void sceScfAdvanceHour(sceCdCLOCK *pClock) {
    unsigned int hour;

    assert(pClock != NULL);
    hour = (unsigned int)pClock->hour + 1u;
    if ((hour & 0xffu) != 0x18u) {
        pClock->hour = (unsigned char)hour;
        return;
    }
    pClock->hour = 0;
    sceScfAdvanceDay(pClock);
}

// NTSC-U/C: 0x005f34d0, PAL: 0x005a60d8
void sceScfRewindHour(sceCdCLOCK *pClock) {
    unsigned int hour;

    assert(pClock != NULL);
    hour = pClock->hour;
    if (hour == 0) {
        pClock->hour = 0x17;
        sceScfRewindDay(pClock);
        return;
    }
    pClock->hour = (unsigned char)(hour - 1u);
}

// NTSC-U/C: 0x005f2ee8, PAL: 0x005a5af0
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

int sceScfGetLanguage(void) {
    unsigned int nConfig;
    unsigned int nVersion;

    GetOsdConfigParam(&nConfig); // Yes, the binary reads the word before the tool check too.
    if (sceScfEnsureRomVersionRead() != 0) {
        return g_nScfDefaultLanguage;
    }
    GetOsdConfigParam(&nConfig);
    nVersion = (nConfig >> kVersionShift) & (unsigned int)kVersionMask;
    if (nVersion == 0u) {
        return (int)((nConfig >> kLegacyLanguageShift) & 1u);
    }
    return (int)((nConfig >> kLanguageShift) & (unsigned int)kLanguageMask);
}

// NTSC-U/C: 0x005f2fd0, PAL: 0x005a5bd8
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

// NTSC-U/C: 0x005f3538, PAL: 0x005a6140
void sceScfApplyMinuteOffset(sceCdCLOCK *pClock, int nMinutes) {
    int nTotal;

    assert(pClock != NULL);
    assert((unsigned int)(nMinutes + kHalfDayMinutes) < (unsigned int)kOffsetRangeWidth);
    sceScfClockFromBcd(pClock);
    nTotal = pClock->minute + nMinutes;
    if (nTotal < 0) {
        do {
            nTotal += kMinutesPerHour;
            sceScfRewindHour(pClock);
        } while (nTotal < 0);
        pClock->minute = (unsigned char)nTotal;
    } else if (nTotal < kHourThreshold) {
        pClock->minute = (unsigned char)nTotal;
    } else {
        do {
            nTotal -= kMinutesPerHour;
            sceScfAdvanceHour(pClock);
        } while (nTotal >= kHourThreshold);
        pClock->minute = (unsigned char)nTotal;
    }
    sceScfClockToBcd(pClock);
}

void sceScfGetLocalTimefromRTC(sceCdCLOCK *pClock) {
    int nTimezone = sceScfGetTimezone();
    int nSummer = sceScfGetSummerTime();
    int nOffset = nTimezone + nSummer * kMinutesPerHour - kTokyoMinutes;

    assert(pClock != NULL);
    sceScfApplyMinuteOffset(pClock, nOffset);
}
