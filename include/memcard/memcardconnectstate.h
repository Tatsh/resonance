#pragma once

#include "os/hxstr.h"

/**
 * What one card slot reports about itself.
 *
 * A plain record of 0x18 bytes with no behaviour and no RTTI. `GetConnectStateMCT` stores one
 * inline and passes it by value to `MemcardUser::OnConnectState()`, which is the only place the
 * record appears. The class title is inferred from `GetConnectStateMCT`, and no string in the
 * image identifies it.
 *
 * Every member is filled by `GetConnectStateMCT::OnCheckInfo()` at `0x00177fb0` from one
 * `CheckInfoOp`.
 */
struct MemcardConnectState {
    int mPortSlot;   /*!< The packed port and slot the state describes. +0x00 */
    HxStr mSlotName; /*!< Display text for the slot, from the table at `0x0067bfe0`. +0x04 */
    int mFree;       /*!< Free clusters, from `CheckInfoOp::mFree`. +0x0c */
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
