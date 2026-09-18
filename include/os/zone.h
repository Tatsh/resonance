#pragma once

/**
 * The number of zones the allocator can create at once.
 *
 * Every loop over the table is written against this count, and FreeAllZones()
 * clears `kZoneCount * sizeof(Zone)` bytes in one call.
 */
constexpr int kZoneCount = 12;

/** The number of bytes a zone name may occupy, including the terminator. */
constexpr int kZoneNameSize = 16;

/** Reported in place of a zone index when no zone applies. */
constexpr int kNoZone = -1;

/**
 * One named region the allocator hands blocks out of.
 *
 * A zone is a single tagged allocation that is then carved up by a bump
 * pointer. Individual allocations are never released, so a zone is reclaimed
 * either by rewinding the cursor with ZoneResetZone() or by releasing the whole
 * region with ZoneDelete().
 *
 * `mBlock` doubles as the slot's occupancy marker. A null `mBlock` marks a free
 * slot.
 */
struct Zone {
    void *mBlock; /*!< The tagged allocation, null while the slot is free. +0x00 */
    char *mStart; /*!< mBlock rounded up to the 64-byte alignment. +0x04 */
    char *mCur;   /*!< The bump pointer, rewound to mStart by ZoneResetZone(). +0x08 */
    int mSize;    /*!< The usable size, which is the size ZoneCreate() requested. +0x0c */
    char mName[kZoneNameSize]; /*!< The zone name, as ZoneCreate() copied it. +0x10 */
};

/**
 * One entry of the start-up zone list.
 *
 * InitializeZoneList() walks the list until it finds an entry with a null name. The shipped list is
 * nine entries followed by that terminator, and both columns are recovered.
 *
 * | Name              | Size |
 * | ----------------- | ---- |
 * | `seccache`        | 512  |
 * | `rndfile`         | 660  |
 * | `rndglobal`       | 1290 |
 * | `rndCommon`       | 1350 |
 * | `rndTnlLevel`     | 450  |
 * | `rndTnlArena`     | 490  |
 * | `movieStreamBuff` | 1024 |
 * | `python`          | 2400 |
 * | `temp`            | 128  |
 *
 * Three of those sizes corroborate constants recovered elsewhere. `temp` at 128 KiB matches the
 * fallback ZoneGrabTemp() allocates when zones are switched off, `seccache` at 512 KiB divides by
 * the 64 KiB row size into exactly the eight rows ArkFile::Open() requests as its own fallback,
 * and `python` at 2400 KiB exceeds the 2 MiB fallback Py_Initialize() passes to ZoneGetAvail().
 */
struct ZoneConfig {
    const char *mName; /*!< The zone name, null on the terminating entry. +0x00 */
    int mSizeKb;       /*!< The size in kibibytes, shifted into bytes by the caller. +0x04 */
};

/**
 * Create a named zone.
 *
 * The region is over-allocated by 63 bytes so the usable start can be rounded
 * up to a 64-byte boundary. A duplicate name, an exhausted table, and a failed
 * allocation are all reported through the log and produce kNoZone.
 *
 * @param pszName The zone name, which must fit in kZoneNameSize bytes with its
 * terminator.
 * @param nSize The usable size in bytes.
 * @return The zone index, or kNoZone on failure and when zones are switched
 * off.
 * @ghidraAddress 0x00461190
 */
int ZoneCreate(const char *pszName, int nSize);

/**
 * Allocate from the current zone.
 *
 * The request is rounded up to 16 bytes and taken by advancing the zone cursor.
 * Exhausting the zone is fatal. With zones switched off, and while no zone is
 * current, the request goes to MemAllocTagged() instead.
 *
 * @param nSize The request in bytes.
 * @return The block, or null when the current zone slot is empty.
 * @ghidraAddress 0x004612e0
 */
void *ZoneAlloc(unsigned nSize);

/**
 * Take the shared temporary buffer.
 *
 * The buffer is the whole of the zone titled `temp`, or a 128 KiB tagged
 * allocation when zones are switched off. It is claimed once and stays claimed
 * until ZoneReleaseTemp() runs, and a second claim is fatal.
 *
 * @return The buffer.
 * @ghidraAddress 0x004613d0
 */
void *ZoneGrabTemp();

/**
 * Turn zone allocation on or off.
 *
 * Switching the setting while zones are on releases every zone first.
 *
 * @param nEnabled Non-zero to allow zones to be created.
 * @ghidraAddress 0x00461518
 */
void SetZonesEnabled(int nEnabled);

/**
 * Release every zone and clear the table.
 *
 * The whole table is zeroed in one pass, which is what distinguishes this
 * routine from ReleaseAllZoneSlots(). The enabled flag is not consulted.
 *
 * @ghidraAddress 0x00461558
 */
void FreeAllZones();

/**
 * Create every zone on the start-up list.
 *
 * @ghidraAddress 0x004615d8
 */
void InitializeZoneList();

/**
 * Release one zone and free its table slot.
 *
 * Deleting the current zone also clears the current-zone selection.
 *
 * @param nZone The zone index.
 * @ghidraAddress 0x00461628
 */
void ZoneDelete(int nZone);

/**
 * Release every zone one slot at a time.
 *
 * Each occupied slot goes through the ZoneDelete() path, which respects the
 * enabled flag and clears the current-zone selection.
 *
 * @ghidraAddress 0x004616c8
 */
void ReleaseAllZoneSlots();

/**
 * Find a zone by name.
 *
 * @param pszName The zone name.
 * @return The zone index, or kNoZone when no zone has that name.
 * @ghidraAddress 0x00461770
 */
int FindZoneByName(const char *pszName);

/**
 * Select the zone that ZoneAlloc() allocates from.
 *
 * The index is stored without a range check. Passing kNoZone routes later
 * requests to MemAllocTagged().
 *
 * @param nZone The zone index.
 * @ghidraAddress 0x004617f8
 */
void ZoneSetCurrent(int nZone);

/**
 * Report the selected zone.
 *
 * @return The zone index, or kNoZone when no zone is selected.
 * @ghidraAddress 0x00461818
 */
int ZoneGetCurrent();

/**
 * Rewind the selected zone, discarding everything allocated from it.
 *
 * @ghidraAddress 0x00461828
 */
void ZoneReset();

/**
 * Rewind one zone, discarding everything allocated from it.
 *
 * @param nZone The zone index.
 * @ghidraAddress 0x00461850
 */
void ZoneResetZone(int nZone);

/**
 * Release a block that ZoneAlloc() returned.
 *
 * A block cut out of a zone cannot be released on its own, so this does nothing
 * while a zone is selected. The call is forwarded to MemFreeTagged() only on
 * the paths where ZoneAlloc() would have used MemAllocTagged().
 *
 * @param pBlock The block to release.
 * @ghidraAddress 0x004618b0
 */
void ZoneFree(void *pBlock);

/**
 * Report the space left in the selected zone.
 *
 * @param nDefault The value to report when zones are switched off.
 * @return The unused bytes of the selected zone, nDefault when zones are
 * switched off, and zero when no zone is selected or the selected slot is
 * empty.
 * @ghidraAddress 0x00461900
 */
int ZoneGetAvail(int nDefault);

/**
 * Find the zone a block belongs to.
 *
 * @param pBlock The block to locate.
 * @return The zone index, or kNoZone when the block is in no zone.
 * @ghidraAddress 0x00461968
 */
int FindZoneForPointer(const void *pBlock);

/**
 * Give the shared temporary buffer back.
 *
 * The buffer itself is retained for the next claim. Only the claim is cleared.
 *
 * @ghidraAddress 0x004619c8
 */
void ZoneReleaseTemp();

/**
 * Write every occupied zone to the log.
 *
 * @ghidraAddress 0x004619d8
 */
void ZoneDump();

/**
 * The zone table.
 *
 * @ghidraAddress 0x008945a0
 */
extern Zone g_adZones[kZoneCount];

/**
 * The start-up zone list, terminated by an entry with a null name.
 *
 * @ghidraAddress 0x006e9810
 */
extern ZoneConfig g_aZoneConfigs[];
