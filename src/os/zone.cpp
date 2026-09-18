#include "os/zone.h"

#include <stdint.h>
#include <string.h>

#include "os/log.h"
#include "os/mem.h"

// The tag arguments below record the original line numbers of zone.cpp,
// recovered from the constants the calls pass: ZoneCreate at 139, FreeAllZones
// at 74, ZoneDelete at 181, ZoneAlloc at 301 and 304, ZoneFree at 336, and
// ZoneGrabTemp at 399.

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

// 0x006e9808
int g_nZonesEnabled;

// 0x006e980c. The initial value is not recoverable from the image.
// FreeAllZones() and ZoneDelete() are what establish kNoZone at runtime.
int g_nCurrentZone;

// 0x006e9860
void *g_pTempBuffer;

// 0x006e9864
int g_nTempBufferSize;

// 0x006e9868
int g_bTempGrabbed;

char *AlignZoneStart(void *pBlock) {
    uintptr_t nAddress = reinterpret_cast<uintptr_t>(pBlock) + kZoneStartAlignment - 1;
    return reinterpret_cast<char *>(nAddress & ~static_cast<uintptr_t>(kZoneStartAlignment - 1));
}

} // namespace

int ZoneCreate(const char *pszName, int nSize) {
    if (g_nZonesEnabled == 0) {
        return kNoZone;
    }

    (void)strlen(pszName); // Yes, the binary discards this call's result.

    if (FindZoneByName(pszName) != kNoZone) {
        LogPrintf("ZoneCreate: %s is duplicate zone name!\n", pszName);
        return kNoZone;
    }

    Zone *pZone = nullptr;
    int nZone = kNoZone;
    for (int i = 0; i < kZoneCount; ++i) {
        if (g_adZones[i].mBlock == nullptr) {
            pZone = &g_adZones[i];
            nZone = i;
            break;
        }
    }
    if (pZone == nullptr) {
        LogPrintf("ZoneCreate: no free zone descriptor slots for zone: %s\n", pszName);
        return kNoZone;
    }

    void *pBlock = MemAllocTagged(nSize + kZoneStartAlignment - 1, __FILE__, __LINE__);
    if (pBlock == nullptr) {
        LogPrintf("ZoneCreate: can't alloc %d bytes for zone: %s\n", nSize, pszName);
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

void *ZoneAlloc(unsigned nSize) {
    if (g_nZonesEnabled == 0) {
        return MemAllocTagged(nSize, __FILE__, __LINE__);
    }
    if (g_nCurrentZone == kNoZone) {
        return MemAllocTagged(nSize, __FILE__, __LINE__);
    }

    Zone *pZone = &g_adZones[g_nCurrentZone];
    if (pZone->mBlock == nullptr) {
        // The shipped message titles the wrong routine.
        LogPrintf("ZoneReset: zone %d is not allocated!\n", g_nCurrentZone);
        return nullptr;
    }

    char *pBlock = pZone->mCur;
    unsigned nAligned = (nSize + kZoneAllocAlignment - 1) & ~(kZoneAllocAlignment - 1);
    char *pNext = pBlock + nAligned;
    if (pZone->mStart + pZone->mSize < pNext) {
        Fatal("ZoneAlloc: zone %d (%s) out of space for alloc of size: %d (zone "
              "size: %d)!\n",
              g_nCurrentZone,
              pZone->mName,
              nSize,
              pZone->mSize);
    }
    pZone->mCur = pNext;
    return pBlock;
}

void *ZoneGrabTemp() {
    if (g_pTempBuffer == nullptr) {
        if (g_nZonesEnabled != 0) {
            int nZone = kNoZone;
            for (int i = 0; i < kZoneCount; ++i) {
                if (g_adZones[i].mBlock != nullptr &&
                    strcmp(g_adZones[i].mName, kTempZoneName) == 0) {
                    nZone = i;
                    break;
                }
            }
            // A missing temp zone indexes one record below the table. The shipped
            // code has no guard here.
            Zone *pZone = &g_adZones[nZone];
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

void SetZonesEnabled(int nEnabled) {
    if (g_nZonesEnabled != 0) {
        FreeAllZones();
    }
    g_nZonesEnabled = nEnabled;
}

void FreeAllZones() {
    for (int i = 0; i < kZoneCount; ++i) {
        if (g_adZones[i].mBlock != nullptr) {
            MemFreeTagged(g_adZones[i].mBlock, __FILE__, __LINE__);
        }
    }
    memset(g_adZones, 0, sizeof(g_adZones));
    g_nCurrentZone = kNoZone;
}

void InitializeZoneList() {
    for (const ZoneConfig *pConfig = g_aZoneConfigs; pConfig->mName != nullptr; ++pConfig) {
        ZoneCreate(pConfig->mName, pConfig->mSizeKb * 1024);
    }
}

void ZoneDelete(int nZone) {
    if (g_nZonesEnabled == 0) {
        return;
    }

    Zone *pZone = &g_adZones[nZone];
    if (pZone->mBlock == nullptr) {
        LogPrintf("ZoneDelete: zone %d can't be deleted because it doesn't exist!\n", nZone);
        return;
    }

    MemFreeTagged(pZone->mBlock, __FILE__, __LINE__);
    memset(pZone, 0, sizeof(*pZone));
    if (g_nCurrentZone == nZone) {
        g_nCurrentZone = kNoZone;
    }
}

void ReleaseAllZoneSlots() {
    for (int i = 0; i < kZoneCount; ++i) {
        if (g_adZones[i].mBlock != nullptr) {
            ZoneDelete(i);
        }
    }
}

int FindZoneByName(const char *pszName) {
    for (int i = 0; i < kZoneCount; ++i) {
        if (g_adZones[i].mBlock != nullptr && strcmp(g_adZones[i].mName, pszName) == 0) {
            return i;
        }
    }
    return kNoZone;
}

void ZoneSetCurrent(int nZone) {
    if (g_nZonesEnabled == 0) {
        return;
    }
    g_nCurrentZone = nZone;
}

int ZoneGetCurrent() {
    return g_nCurrentZone;
}

void ZoneReset() {
    ZoneResetZone(g_nCurrentZone);
}

void ZoneResetZone(int nZone) {
    if (g_nZonesEnabled == 0) {
        return;
    }
    if (static_cast<unsigned>(nZone) >= static_cast<unsigned>(kZoneCount)) {
        return;
    }

    Zone *pZone = &g_adZones[nZone];
    if (pZone->mBlock == nullptr) {
        LogPrintf("ZoneResetZone: zone %d is not allocated!\n", nZone);
        return;
    }
    pZone->mCur = pZone->mStart;
}

void ZoneFree(void *pBlock) {
    if (g_nZonesEnabled != 0 && g_nCurrentZone != kNoZone) {
        return;
    }
    MemFreeTagged(pBlock, __FILE__, __LINE__);
}

int ZoneGetAvail(int nDefault) {
    if (g_nZonesEnabled == 0) {
        return nDefault;
    }
    if (static_cast<unsigned>(g_nCurrentZone) >= static_cast<unsigned>(kZoneCount)) {
        return 0;
    }

    Zone *pZone = &g_adZones[g_nCurrentZone];
    if (pZone->mBlock == nullptr) {
        return 0;
    }
    return pZone->mSize - (pZone->mCur - pZone->mStart);
}

int FindZoneForPointer(const void *pBlock) {
    const char *pAddress = static_cast<const char *>(pBlock);
    for (int i = 0; i < kZoneCount; ++i) {
        if (g_adZones[i].mBlock == nullptr) {
            continue;
        }
        if (pAddress >= g_adZones[i].mStart &&
            pAddress < g_adZones[i].mStart + g_adZones[i].mSize) {
            return i;
        }
    }
    return kNoZone;
}

void ZoneReleaseTemp() {
    g_bTempGrabbed = 0;
}

void ZoneDump() {
    if (g_nZonesEnabled == 0) {
        return;
    }

    LogPrintf("ZONE DUMP:\n");
    for (int i = 0; i < kZoneCount; ++i) {
        Zone *pZone = &g_adZones[i];
        if (pZone->mBlock == nullptr) {
            continue;
        }
        LogPrintf("  zone %d (%s):  size: %d, avail: %d (range: %p to %p)\n",
                  i,
                  pZone->mName,
                  pZone->mSize,
                  pZone->mSize - (pZone->mCur - pZone->mStart),
                  pZone->mStart,
                  pZone->mStart + pZone->mSize);
    }
}
