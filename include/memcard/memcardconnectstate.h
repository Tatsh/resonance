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
