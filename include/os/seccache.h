#pragma once

/** The number of bytes each row buffers, which is what the row count divides the zone by. */
constexpr int kSectorCacheRowSize = 0x10000;

/** The space the cache assumes when zones are switched off. */
constexpr int kSectorCacheFallbackSize = 0x80000;

/** Stored in a row's file and sector while the row buffers nothing. */
constexpr int kSectorCacheRowEmpty = -1;

/**
 * Stamp that locks a row against reuse.
 *
 * SectorCacheGetLru() skips a locked row, and neither routine advances a locked row's stamp. The
 * value sorts above every real stamp, so the unsigned comparison in the search excludes it even
 * before the explicit test.
 */
constexpr unsigned kSectorCacheLocked = 0x99999999;

/** Stamp the least-recently-used search starts from, above every stamp the clock produces. */
constexpr unsigned kSectorCacheStampCeiling = 0x7fffffff;

/** A chosen stamp above this triggers the rebase that stops the clock from wrapping. */
constexpr unsigned kSectorCacheStampHalfway = 0x3fffffff;

/** The amount every stamp above the halfway mark is reduced by. */
constexpr unsigned kSectorCacheStampRebase = 0x40000000;

/**
 * One buffered run of a file.
 *
 * Titled after `seccache.cpp`, the file its allocation tag records. A row buffers
 * kSectorCacheRowSize bytes, so a sector here is a 64 KiB chunk rather than a media sector.
 */
struct SectorCacheRow {
    int mFile;       /*!< The buffered file, or kSectorCacheRowEmpty. +0x00 */
    int mSector;     /*!< The buffered chunk index, or kSectorCacheRowEmpty. +0x04 */
    unsigned mStamp; /*!< Access stamp for the least-recently-used search. +0x08 */
    void *mBuffer;   /*!< The kSectorCacheRowSize buffer. +0x0c */
};

/**
 * Bring up the sector cache.
 *
 * The cache takes the whole of the zone titled `seccache` and divides it into as many
 * kSectorCacheRowSize buffers as fit, so the requested row count applies only when zones are
 * switched off. Every row starts empty. ArkFile::Open() is the caller, on the first mount, and it
 * requests eight rows.
 *
 * @param nRows The row count to use when zones are switched off.
 * @ghidraAddress 0x00554fb8
 */
void InitSectorCache(int nRows);

/**
 * Release every row and the row table.
 *
 * A row buffer taken from a zone goes back through ZoneFree(), and one taken from the allocator
 * goes back through MemFreeTagged().
 *
 * @ghidraAddress 0x005550c8
 */
void ShutdownSectorCache();

/**
 * Drop every row that buffers a file.
 *
 * Each matching row has its file and sector reset to kSectorCacheRowEmpty and its stamp cleared,
 * which makes it the oldest row and therefore the first to be reused. The buffers stay allocated.
 *
 * @param nFile The file whose rows should be dropped.
 * @ghidraAddress 0x00555298
 */
void InvalidateCachedSectors(int nFile);

/**
 * Find the row buffering a chunk.
 *
 * A match that is not locked is stamped with the current clock, which makes it the newest row.
 *
 * @param nFile The file.
 * @param nSector The chunk index.
 * @return The row, or null when no row buffers that chunk.
 * @ghidraAddress 0x00555190
 */
SectorCacheRow *SectorCacheFind(int nFile, int nSector);

/**
 * Take the least recently used row for a chunk.
 *
 * The name comes from the routine's own warning text. Locked rows are skipped. Once the chosen
 * row's stamp passes the halfway mark every stamp is rebased downward, which is what stops the
 * clock from wrapping. The row is then pointed at the requested chunk and stamped as newest, and
 * its buffer still holds the previous chunk until a read replaces it.
 *
 * @param nFile The file to buffer.
 * @param nSector The chunk index to buffer.
 * @return The row, or null when every row is locked.
 * @ghidraAddress 0x00554e50
 */
SectorCacheRow *SectorCacheGetLru(int nFile, int nSector);

/**
 * Lock the row buffering a chunk against reuse.
 *
 * The search repeats SectorCacheFind() inline, stamp bump included, and the chosen row then takes
 * kSectorCacheLocked. A miss is reported through LogPrintf() and otherwise ignored. Nothing in the
 * image calls this routine, so it is dead code in the shipped build.
 *
 * @param nFile The buffered file.
 * @param nSector The buffered chunk index.
 * @ghidraAddress 0x005552f0
 */
void LockCachedSector(int nFile, int nSector);

/**
 * Lock one row against reuse.
 *
 * The routine exists out of line because a caller in the asynchronous loader already has the row.
 *
 * @param pRow The row to lock.
 * @ghidraAddress 0x00555398
 */
void SetSectorRowLocked(SectorCacheRow *pRow);

/**
 * Release the lock on the row buffering a chunk.
 *
 * The search repeats SectorCacheFind() inline, whose stamp bump is skipped because a locked row
 * stamp equals kSectorCacheLocked. Writing the current clock into the row both unlocks it and
 * makes it the newest. A miss is reported through LogPrintf() and otherwise ignored.
 *
 * @param nFile The buffered file.
 * @param nSector The buffered chunk index.
 * @ghidraAddress 0x005553a8
 */
void UnlockCachedSector(int nFile, int nSector);

/**
 * Print every row through LogPrintf().
 *
 * @ghidraAddress 0x005554a0
 */
void DumpSectorCache();

/**
 * Number of rows the cache was built with.
 *
 * @ghidraAddress 0x008de790
 */
extern int g_nSectorCacheRows;

/**
 * The row table.
 *
 * @ghidraAddress 0x008de794
 */
extern SectorCacheRow *g_pSectorCacheRows;
