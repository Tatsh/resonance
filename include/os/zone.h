#pragma once

/** The number of zones the allocator can hold at once. */
constexpr int kZoneCount = 12;

/**
 * One named region the allocator hands blocks out of.
 *
 * Only the fields the zone lookup reads have been recovered.
 */
struct Zone {
    void *mBlock;  // +0x00 the backing allocation, null when the slot is free
    char *mStart;  // +0x04
    int mUnknown08; // +0x08
    int mSize;     // +0x0c
};

/**
 * Turn zone allocation on or off.
 *
 * Switching the setting while zones exist releases them all first.
 *
 * @param nEnabled Non-zero to allow zones to be created.
 * @ghidraAddress 0x00461518
 */
void SetZonesEnabled(int nEnabled);

/**
 * Release every zone and clear the table.
 *
 * @ghidraAddress 0x00461558
 */
void FreeAllZones();

/**
 * Find the zone a block belongs to.
 *
 * @param pBlock The block to locate.
 * @return The zone index, or -1 when the block is not in any zone.
 * @ghidraAddress 0x00461968
 */
int FindZoneForPointer(const void *pBlock);

/**
 * Find a zone by name.
 *
 * @param pszName The zone name.
 * @return The zone index, or -1 when no zone has that name.
 * @ghidraAddress 0x00461770
 */
int FindZoneByName(const char *pszName);

/**
 * The zone table.
 *
 * @ghidraAddress 0x008945a0
 */
extern Zone g_aZones[kZoneCount];
