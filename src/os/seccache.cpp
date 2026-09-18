#include "os/seccache.h"

#include "os/log.h"
#include "os/mem.h"
#include "os/zone.h"

namespace {

// The zone the cache carves its row buffers out of.
constexpr char kSectorCacheZoneName[] = "seccache";

// 0x008de798. The access clock, not a flag. Stamps start at 1 so that a row that has never been
// used, whose stamp is zero, always sorts as the oldest.
unsigned g_nSectorCacheClock;

// 0x00724bf0. Nothing anywhere in the image writes this word. The read below is its only
// reference, so it is permanently zero and the ZoneFree() branch it selects is dead code.
int g_nSectorCacheZone;

} // namespace

int g_nSectorCacheRows;
SectorCacheRow *g_pSectorCacheRows;

void InitSectorCache(int nRows) {
    int nSaved = ZoneGetCurrent();
    int nZone = FindZoneByName(kSectorCacheZoneName);
    ZoneSetCurrent(nZone);
    ZoneReset();

    if (nZone == kNoZone) {
        g_nSectorCacheRows = nRows;
    } else {
        g_nSectorCacheRows = ZoneGetAvail(kSectorCacheFallbackSize) / kSectorCacheRowSize;
    }

    g_pSectorCacheRows = static_cast<SectorCacheRow *>(
        MemAllocTagged(g_nSectorCacheRows * sizeof(SectorCacheRow), __FILE__, __LINE__));
    g_nSectorCacheClock = 1;

    SectorCacheRow *pRow = g_pSectorCacheRows;
    for (int i = 0; i < g_nSectorCacheRows; ++i) {
        pRow->mFile = kSectorCacheRowEmpty;
        pRow->mSector = kSectorCacheRowEmpty;
        pRow->mStamp = 0;
        pRow->mBuffer = ZoneAlloc(kSectorCacheRowSize);
        ++pRow;
    }

    ZoneSetCurrent(nSaved);
}

void ShutdownSectorCache() {
    SectorCacheRow *pRow = g_pSectorCacheRows;
    for (int i = 0; i < g_nSectorCacheRows; ++i) {
        if (g_nSectorCacheZone != kNoZone) {
            // Unreachable. The word is never written, so this branch never runs even though the
            // buffers do come from a zone.
            ZoneFree(pRow->mBuffer);
        } else {
            MemFreeTagged(pRow->mBuffer, __FILE__, __LINE__);
        }
        pRow->mBuffer = nullptr;
        ++pRow;
    }

    MemFreeTagged(g_pSectorCacheRows, __FILE__, __LINE__);
    g_pSectorCacheRows = nullptr;
    g_nSectorCacheRows = 0;
}

void InvalidateCachedSectors(int nFile) {
    for (int i = 0; i < g_nSectorCacheRows; ++i) {
        SectorCacheRow *pRow = &g_pSectorCacheRows[i];
        if (pRow->mFile == nFile) {
            pRow->mFile = kSectorCacheRowEmpty;
            pRow->mSector = kSectorCacheRowEmpty;
            pRow->mStamp = 0;
        }
    }
}

SectorCacheRow *SectorCacheFind(int nFile, int nSector) {
    for (int i = 0; i < g_nSectorCacheRows; ++i) {
        SectorCacheRow *pRow = &g_pSectorCacheRows[i];
        if (pRow->mFile != nFile || pRow->mSector != nSector) {
            continue;
        }
        if (pRow->mStamp != kSectorCacheLocked) {
            pRow->mStamp = g_nSectorCacheClock;
            ++g_nSectorCacheClock;
        }
        return pRow;
    }
    return nullptr;
}

SectorCacheRow *SectorCacheGetLru(int nFile, int nSector) {
    unsigned nOldest = kSectorCacheStampCeiling;
    SectorCacheRow *pChosen = nullptr;
    SectorCacheRow *pRow = g_pSectorCacheRows;
    for (int i = g_nSectorCacheRows; i != 0; --i) {
        // The unsigned comparison already excludes a locked row, because the locked stamp sorts
        // above the ceiling. The second test is the shipped code's own belt and braces.
        if (pRow->mStamp < nOldest && pRow->mStamp != kSectorCacheLocked) {
            nOldest = pRow->mStamp;
            pChosen = pRow;
        }
        ++pRow;
    }
    if (pChosen == nullptr) {
        return nullptr;
    }

    if (pChosen->mStamp > kSectorCacheStampHalfway && pChosen->mStamp != kSectorCacheStampCeiling) {
        // The clock has run far enough that stamps are rebased downward rather than allowed to
        // wrap, which preserves their order.
        pRow = g_pSectorCacheRows;
        for (int i = 0; i < g_nSectorCacheRows; ++i) {
            if (pRow->mStamp > kSectorCacheStampHalfway && pRow->mStamp != kSectorCacheLocked) {
                pRow->mStamp -= kSectorCacheStampRebase;
            }
            ++pRow;
        }
        if (g_nSectorCacheClock > kSectorCacheStampHalfway) {
            g_nSectorCacheClock -= kSectorCacheStampRebase;
        }
    }

    pChosen->mFile = nFile;
    pChosen->mSector = nSector;
    if (pChosen->mStamp == kSectorCacheLocked) {
        // Unreachable, because the search above never chooses a locked row.
        LogPrintf("ARRGHGHGHGH - SECTORCACHEGETLRU GOT LOCKED SECTOR\n");
    }
    pChosen->mStamp = g_nSectorCacheClock;
    ++g_nSectorCacheClock;
    return pChosen;
}
