#include "os/seccache.h"

#include "os/log.h"
#include "os/mem.h"
#include "os/zone.h"

namespace {

// The zone the cache carves its row buffers out of.
constexpr char kSectorCacheZoneName[] = "seccache";

// The access clock, not a flag. Stamps start at 1 so that a row that has never been used, whose
// stamp is zero, always sorts as the oldest.
// NTSC-U/C: 0x008de798, PAL: 0x00923758
unsigned gCurrTimestamp;

// The image initialises this word to kNoZone and nothing writes it. The read below is its only
// reference, so the ZoneFree() branch it selects is dead code.
// NTSC-U/C: 0x00724bf0, PAL: 0x007687e0
int g_nSectorCacheZone = kNoZone;

} // namespace

SectorCache gSectorCache;

// NTSC-U/C: 0x00554fb8, PAL: 0x00595640
void SectorCacheInit(int nRows) {
    int nSaved = ZoneGetCurrent();
    int nZone = FindZoneByName(kSectorCacheZoneName);
    ZoneSetCurrent(nZone);
    ZoneReset();

    if (nZone == kNoZone) {
        gSectorCache.mRowCount = nRows;
    } else {
        gSectorCache.mRowCount = ZoneGetAvail(kSectorCacheFallbackSize) / kSectorCacheRowSize;
    }

    gSectorCache.mRows = static_cast<SectorCacheRow *>(
        MemAllocTagged(gSectorCache.mRowCount * sizeof(SectorCacheRow), __FILE__, __LINE__));
    gCurrTimestamp = 1;

    SectorCacheRow *pRow = gSectorCache.mRows;
    for (int i = 0; i < gSectorCache.mRowCount; ++i) {
        pRow->mFile = kSectorCacheRowEmpty;
        pRow->mSector = kSectorCacheRowEmpty;
        pRow->mStamp = 0;
        pRow->mBuffer = ZoneAlloc(kSectorCacheRowSize);
        ++pRow;
    }

    ZoneSetCurrent(nSaved);
}

// NTSC-U/C: 0x005550c8, PAL: 0x00595750
void SectorCacheTerm() {
    SectorCacheRow *pRow = gSectorCache.mRows;
    for (int i = 0; i < gSectorCache.mRowCount; ++i) {
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

    MemFreeTagged(gSectorCache.mRows, __FILE__, __LINE__);
    gSectorCache.mRows = nullptr;
    gSectorCache.mRowCount = 0;
}

// NTSC-U/C: 0x00555298, PAL: 0x00595920
void SectorCacheRemove(int nFile) {
    for (int i = 0; i < gSectorCache.mRowCount; ++i) {
        SectorCacheRow *pRow = &gSectorCache.mRows[i];
        if (pRow->mFile == nFile) {
            pRow->mFile = kSectorCacheRowEmpty;
            pRow->mSector = kSectorCacheRowEmpty;
            pRow->mStamp = 0;
        }
    }
}

// NTSC-U/C: 0x00555190, PAL: 0x00595818
SectorCacheRow *SectorCacheFind(int nFile, int nSector) {
    for (int i = 0; i < gSectorCache.mRowCount; ++i) {
        SectorCacheRow *pRow = &gSectorCache.mRows[i];
        if (pRow->mFile != nFile || pRow->mSector != nSector) {
            continue;
        }
        if (pRow->mStamp != kSectorCacheLocked) {
            pRow->mStamp = gCurrTimestamp;
            ++gCurrTimestamp;
        }
        return pRow;
    }
    return nullptr;
}

// NTSC-U/C: 0x005552f0, PAL: 0x00595978
void SectorCacheLock(int nFile, int nSector) {
    // The inlined search advances the clock over the row it finds, and the sentinel below then
    // replaces the value it wrote. One clock tick is therefore spent for nothing.
    SectorCacheRow *pRow = SectorCacheFind(nFile, nSector);
    if (pRow == nullptr) {
        printf("UNABLE TO LOCK SECTOR %d\n", nSector);
        return;
    }
    pRow->mStamp = kSectorCacheLocked;
}

// NTSC-U/C: 0x00555398, PAL: 0x00595a20
void SetSectorRowLocked(SectorCacheRow *pRow) {
    pRow->mStamp = kSectorCacheLocked;
}

// NTSC-U/C: 0x005553a8, PAL: 0x00595a30
void UnlockCachedSector(int nFile, int nSector) {
    SectorCacheRow *pRow = SectorCacheFind(nFile, nSector);
    if (pRow == nullptr) {
        printf("CAN'T FIND SECTOR IN CACHE TO UNLOCK!!!!\n");
        return;
    }
    pRow->mStamp = gCurrTimestamp;
    ++gCurrTimestamp;
}

// NTSC-U/C: 0x005554a0, PAL: 0x00595b28
void SectorCacheDump() {
    printf("SECTOR CACHE:\n");
    for (int i = 0; i < gSectorCache.mRowCount; ++i) {
        SectorCacheRow *pRow = &gSectorCache.mRows[i];
        printf("%d:  id:%d, sector:%d, timestamp:$%x, p:%p\n",
               i,
               pRow->mFile,
               pRow->mSector,
               pRow->mStamp,
               pRow->mBuffer);
    }
}

// NTSC-U/C: 0x00554e50, PAL: 0x005954d8
SectorCacheRow *SectorCacheGetLRU(int nFile, int nSector) {
    unsigned nOldest = kSectorCacheStampCeiling;
    SectorCacheRow *pChosen = nullptr;
    SectorCacheRow *pRow = gSectorCache.mRows;
    for (int i = gSectorCache.mRowCount; i > 0; --i) {
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
        pRow = gSectorCache.mRows;
        for (int i = 0; i < gSectorCache.mRowCount; ++i) {
            if (pRow->mStamp > kSectorCacheStampHalfway && pRow->mStamp != kSectorCacheLocked) {
                pRow->mStamp -= kSectorCacheStampRebase;
            }
            ++pRow;
        }
        if (gCurrTimestamp > kSectorCacheStampHalfway) {
            gCurrTimestamp -= kSectorCacheStampRebase;
        }
    }

    pChosen->mFile = nFile;
    pChosen->mSector = nSector;
    if (pChosen->mStamp == kSectorCacheLocked) {
        // Unreachable, because the search above never chooses a locked row.
        printf("ARRGHGHGHGH - SECTORCACHEGETLRU GOT LOCKED SECTOR\n");
    }
    pChosen->mStamp = gCurrTimestamp;
    ++gCurrTimestamp;
    return pChosen;
}
