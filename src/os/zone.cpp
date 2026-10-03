#include "os/zone.h"

#include <stdint.h>
#include <string.h>

#include "os/log.h"
#include "os/mem.h"

namespace {

// A zone's usable start is rounded up to a cache-line boundary, which is why
// ZoneCreate() over-allocates by one boundary less a byte.
constexpr int kZoneStartAlignment = 64;

// Every request out of a zone is rounded up to this many bytes.
constexpr int kZoneAllocAlignment = 16;

// The size of the temporary buffer ZoneGrabTemp() allocates when zones are
// switched off.
constexpr int kTempBufferFallbackSize = 128 * 1024;

// The zone ZoneGrabTemp() takes its buffer from.
constexpr char kTempZoneName[] = "temp";

// NTSC-U/C: 0x006e9808, PAL: 0x0072d1a0
int bZonesInUse = 0;

// NTSC-U/C: 0x006e980c, PAL: 0x0072d1a4
int zhCurr = kNoZone;

// NTSC-U/C: 0x006e9860, PAL: 0x0072d1f8
void *g_pTempBuffer = nullptr;

// NTSC-U/C: 0x006e9864, PAL: 0x0072d1fc
int g_nTempBufferSize = 0;

// NTSC-U/C: 0x006e9868, PAL: 0x0072d200
int g_bTempGrabbed = 0;

char *AlignZoneStart(void *pBlock) {
    uintptr_t nAddress = reinterpret_cast<uintptr_t>(pBlock) + kZoneStartAlignment - 1;
    return reinterpret_cast<char *>(nAddress & ~static_cast<uintptr_t>(kZoneStartAlignment - 1));
}

} // namespace

// NTSC-U/C: 0x00461190, PAL: 0x0049e850
int ZoneCreate(const char *pszName, int nSize) {
    if (bZonesInUse == 0) {
        return kNoZone;
    }

    (void)strlen(pszName); // Yes, the binary discards this call's result.

    if (FindZoneByName(pszName) != kNoZone) {
        printf("ZoneCreate: %s is duplicate zone name!\n", pszName);
        return kNoZone;
    }

    Zone *pZone = nullptr;
    int nZone = kNoZone;
    for (int i = 0; i < kZoneCount; ++i) {
        if (zoneDescs[i].mBlock == nullptr) {
            pZone = &zoneDescs[i];
            nZone = i;
            break;
        }
    }
    if (pZone == nullptr) {
        printf("ZoneCreate: no free zone descriptor slots for zone: %s\n", pszName);
        return kNoZone;
    }

    void *pBlock = MemAllocTagged(nSize + kZoneStartAlignment - 1, __FILE__, __LINE__);
    if (pBlock == nullptr) {
        printf("ZoneCreate: can't alloc %d bytes for zone: %s\n", nSize, pszName);
        return kNoZone;
    }

    char *pStart = AlignZoneStart(pBlock);
    pZone->mSize = nSize;
    pZone->mCur = pStart;
    pZone->mBlock = pBlock;
    pZone->mStart = pStart;
    strcpy(pZone->mName, pszName);
    return nZone;
}

// NTSC-U/C: 0x004612e0, PAL: 0x0049e9a0
void *ZoneAlloc(unsigned nSize) {
    if (bZonesInUse == 0) {
        return MemAllocTagged(nSize, __FILE__, __LINE__);
    }
    if (zhCurr == kNoZone) {
        return MemAllocTagged(nSize, __FILE__, __LINE__);
    }

    Zone *pZone = &zoneDescs[zhCurr];
    if (pZone->mBlock == nullptr) {
        // The shipped message identifies the wrong routine.
        printf("ZoneReset: zone %d is not allocated!\n", zhCurr);
        return nullptr;
    }

    char *pBlock = pZone->mCur;
    unsigned nAligned = (nSize + kZoneAllocAlignment - 1) & ~(kZoneAllocAlignment - 1);
    char *pNext = pBlock + nAligned;
    if (pZone->mStart + pZone->mSize < pNext) {
        Fatal("ZoneAlloc: zone %d (%s) out of space for alloc of size: %d (zone "
              "size: %d)!\n",
              zhCurr,
              pZone->mName,
              nSize,
              pZone->mSize);
    }
    pZone->mCur = pNext;
    return pBlock;
}

// NTSC-U/C: 0x004613d0, PAL: 0x0049ea90
void *ZoneGrabTemp([[maybe_unused]] int nSize) {
    if (g_pTempBuffer == nullptr) {
        if (bZonesInUse != 0) {
            int nZone = kNoZone;
            for (int i = 0; i < kZoneCount; ++i) {
                if (zoneDescs[i].mBlock != nullptr &&
                    strcmp(zoneDescs[i].mName, kTempZoneName) == 0) {
                    nZone = i;
                    break;
                }
            }
            // A missing temp zone indexes one record below the table. The shipped
            // code has no guard here.
            Zone *pZone = &zoneDescs[nZone];
            g_pTempBuffer = pZone->mStart;
            g_nTempBufferSize = pZone->mSize;
        } else {
            g_nTempBufferSize = kTempBufferFallbackSize;
            g_pTempBuffer = MemAllocTagged(kTempBufferFallbackSize, __FILE__, __LINE__);
        }
    }

    if (g_pTempBuffer == nullptr) {
        Fatal("ZoneGrabTemp: can't allocate temp buffer!\n");
    }
    if (g_bTempGrabbed != 0) {
        Fatal("ZoneGrabTemp: temp memory already grabbed!\n");
    }
    g_bTempGrabbed = 1;
    return g_pTempBuffer;
}

// NTSC-U/C: 0x00461518, PAL: 0x0049ebd8
void ZoneInit(int nEnabled) {
    if (bZonesInUse != 0) {
        FreeAllZones();
    }
    bZonesInUse = nEnabled;
}

// NTSC-U/C: 0x00461558, PAL: 0x0049ec18
void FreeAllZones() {
    for (int i = 0; i < kZoneCount; ++i) {
        if (zoneDescs[i].mBlock != nullptr) {
            MemFreeTagged(zoneDescs[i].mBlock, __FILE__, __LINE__);
        }
    }
    memset(zoneDescs, 0, sizeof(zoneDescs));
    zhCurr = kNoZone;
}

// NTSC-U/C: 0x004615d8, PAL: 0x0049ec98
void InitializeZoneList() {
    for (const ZoneConfig *pConfig = g_aZoneConfigs; pConfig->mName != nullptr; ++pConfig) {
        ZoneCreate(pConfig->mName, pConfig->mSizeKb * 1024);
    }
}

// NTSC-U/C: 0x00461628, PAL: 0x0049ece8
void ZoneDelete(int nZone) {
    if (bZonesInUse == 0) {
        return;
    }

    Zone *pZone = &zoneDescs[nZone];
    if (pZone->mBlock == nullptr) {
        printf("ZoneDelete: zone %d can't be deleted because it doesn't exist!\n", nZone);
        return;
    }

    MemFreeTagged(pZone->mBlock, __FILE__, __LINE__);
    memset(pZone, 0, sizeof(*pZone));
    if (zhCurr == nZone) {
        zhCurr = kNoZone;
    }
}

// NTSC-U/C: 0x004616c8, PAL: 0x0049ed88
void ReleaseAllZoneSlots() {
    for (int i = 0; i < kZoneCount; ++i) {
        if (zoneDescs[i].mBlock != nullptr) {
            ZoneDelete(i);
        }
    }
}

// NTSC-U/C: 0x00461770, PAL: 0x0049ee30
int FindZoneByName(const char *pszName) {
    for (int i = 0; i < kZoneCount; ++i) {
        if (zoneDescs[i].mBlock != nullptr && strcmp(zoneDescs[i].mName, pszName) == 0) {
            return i;
        }
    }
    return kNoZone;
}

// NTSC-U/C: 0x004617f8, PAL: 0x0049eeb8
void ZoneSetCurrent(int nZone) {
    if (bZonesInUse == 0) {
        return;
    }
    zhCurr = nZone;
}

// NTSC-U/C: 0x00461818, PAL: 0x0049eed8
int ZoneGetCurrent() {
    return zhCurr;
}

// NTSC-U/C: 0x00461828, PAL: 0x0049eee8
void ZoneReset() {
    ZoneResetZone(zhCurr);
}

// NTSC-U/C: 0x00461850, PAL: 0x0049ef10
void ZoneResetZone(int nZone) {
    if (bZonesInUse == 0) {
        return;
    }
    if (static_cast<unsigned>(nZone) >= static_cast<unsigned>(kZoneCount)) {
        return;
    }

    Zone *pZone = &zoneDescs[nZone];
    if (pZone->mBlock == nullptr) {
        printf("ZoneResetZone: zone %d is not allocated!\n", nZone);
        return;
    }
    pZone->mCur = pZone->mStart;
}

// NTSC-U/C: 0x004618b0, PAL: 0x0049ef70
void ZoneFree(void *pBlock) {
    if (bZonesInUse != 0 && zhCurr != kNoZone) {
        return;
    }
    MemFreeTagged(pBlock, __FILE__, __LINE__);
}

// NTSC-U/C: 0x00461900, PAL: 0x0049efc0
int ZoneGetAvail(int nDefault) {
    if (bZonesInUse == 0) {
        return nDefault;
    }
    if (static_cast<unsigned>(zhCurr) >= static_cast<unsigned>(kZoneCount)) {
        return 0;
    }

    Zone *pZone = &zoneDescs[zhCurr];
    if (pZone->mBlock == nullptr) {
        return 0;
    }
    return pZone->mSize - (pZone->mCur - pZone->mStart);
}

// NTSC-U/C: 0x00461968, PAL: 0x0049f028
int FindZoneForPointer(const void *pBlock) {
    const char *pAddress = static_cast<const char *>(pBlock);
    for (int i = 0; i < kZoneCount; ++i) {
        if (zoneDescs[i].mBlock == nullptr) {
            continue;
        }
        if (pAddress >= zoneDescs[i].mStart &&
            pAddress < zoneDescs[i].mStart + zoneDescs[i].mSize) {
            return i;
        }
    }
    return kNoZone;
}

// NTSC-U/C: 0x004619c8, PAL: 0x0049f088
void ZoneReleaseTemp() {
    g_bTempGrabbed = 0;
}

// NTSC-U/C: 0x004619d8, PAL: 0x0049f098
void ZoneDump() {
    if (bZonesInUse == 0) {
        return;
    }

    printf("ZONE DUMP:\n");
    for (int i = 0; i < kZoneCount; ++i) {
        Zone *pZone = &zoneDescs[i];
        if (pZone->mBlock == nullptr) {
            continue;
        }
        printf("  zone %d (%s):  size: %d, avail: %d (range: %p to %p)\n",
               i,
               pZone->mName,
               pZone->mSize,
               pZone->mSize - (pZone->mCur - pZone->mStart),
               pZone->mStart,
               pZone->mStart + pZone->mSize);
    }
}

Zone zoneDescs[kZoneCount];

// The European release enlarges rndglobal for the fonts and screens of its five languages.
// NTSC-U/C: 0x006e9810, PAL: 0x0072d1a8
ZoneConfig g_aZoneConfigs[] = {
    {"seccache", 512},
    {"rndfile", 660},
#ifdef VIDEO_STANDARD_PAL
    {"rndglobal", 1590},
#else
    {"rndglobal", 1290},
#endif
    {"rndCommon", 1350},
    {"rndTnlLevel", 450},
    {"rndTnlArena", 490},
    {"movieStreamBuff", 1024},
    {"python", 2400},
    {"temp", 128},
    {nullptr, 0},
};
