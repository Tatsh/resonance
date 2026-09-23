#pragma once

#include "os/hxstr.h"

/**
 * What one card slot reports about itself, and the memory-card location the front end stores.
 *
 * A plain record of 0x18 bytes with no RTTI. `GetConnectStateMCT` stores one inline and passes it
 * by value to `MemcardUser::OnConnectState()`. The front end keeps the same record as its card
 * location. GlobalSettings::mCardSlots is a vector of them, MetSaveRemix::OnConnectState() at
 * `0x00372c10` assigns a reported state into its first entry, and NextCardSlot() and
 * MetRemixManager::ListRemixes() take them. The class title is inferred from `GetConnectStateMCT`,
 * and no string in the image identifies it.
 *
 * mPortSlot combines the port in its high byte with the multitap slot in its low byte, which
 * NextCardSlot() establishes: `1` and `1-A` are 0, `1-B` is 1, and `2` is 0x100.
 *
 * Every member is filled by `GetConnectStateMCT::OnCheckInfo()` at `0x00177fb0` from one
 * `CheckInfoOp`.
 */
struct MemcardConnectState {
    /**
     * Start with no location and an empty name.
     *
     * Inline. The `GetConnectStateMCT` constructor, NextCardSlot(), and the screens that hold one
     * expand it in place.
     */
    MemcardConnectState() : mPortSlot(-1), mSlotName(""), mFree(-1), mType(-1), mFormatted(0) {
    }

    int mPortSlot;   /*!< The packed port and slot the state describes, or -1. +0x00 */
    HxStr mSlotName; /*!< Display text for the slot, from the table at `0x0067bfe0`. +0x04 */
    int mFree;       /*!< Free clusters, from `CheckInfoOp::mFree`. -1 by default. +0x0c */
    int mType;       /*!< The `sceMcType*` value, from `CheckInfoOp::mType`. +0x10 */
    int mFormatted;  /*!< Non-zero when `CheckInfoOp::mFormatted` was exactly 1. +0x14 */
};

/** Entries in the parallel slot tables, two ports of four multi-tap slots plus the two bare slots.
 */
constexpr int kMemcardSlotCount = 10;

/**
 * Packed port and slot for each enumerable slot.
 *
 * Index 0 and 1 are port 1 and port 2 with no multi-tap. Index 2 to 5 are the four multi-tap slots
 * of port 1, and index 6 to 9 those of port 2. `GetAllConnectStatesMCT` uses index 0 to 5 and never
 * index 6 to 9, so the last four entries are unreferenced.
 *
 * @ghidraAddress 0x007db440
 */
extern const int g_anMemcardSlotPortSlot[kMemcardSlotCount];

/**
 * Display text for each enumerable slot, parallel to g_anMemcardSlotPortSlot.
 *
 * `1`, `2`, then `1-A` to `1-D` and `2-A` to `2-D`. The last four are reached by no code path.
 *
 * @ghidraAddress 0x0067bfe0
 */
extern const char *const g_apszMemcardSlotNames[kMemcardSlotCount];

/** Controller port the first memory-card slot sits in. */
constexpr int kMemcardPort1 = 0;

/** Controller port the second memory-card slot sits in. */
constexpr int kMemcardPort2 = 1;

/** Slots `sceMcGetSlotMax()` reports for a port with no multi-tap. */
constexpr int kMemcardSlotsWithoutMultiTap = 1;
