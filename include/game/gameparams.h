#pragma once

#include "os/hxstr.h"

/**
 * Settings a game session is started with.
 *
 * `10GameParams` in the RTTI descriptor at `0x0086f628`, with no base, so the compiler places the
 * vptr after the data at `+0x34` and the class is 0x38 bytes. Its vtable is at `0x007db5a0`.
 *
 * The whole layout comes from the copy constructor at `0x001fc480`, which copies every member in
 * order, so the offsets and widths are recovered and the three HxStr members are certain. The
 * purpose of each member is not recovered, and no reader has been traced, so every member is
 * private.
 *
 * This class is declared because several network packets embed one. It is the only one of the
 * three such member classes whose layout falls out of its copy constructor; PlayerInfo and
 * FreqAppearance both own heap state and need more work.
 */
class GameParams {
public:
    /**
     * @param other The settings to copy.
     * @ghidraAddress 0x001fc480
     */
    GameParams(const GameParams &other);

private:
    HxStr mUnknown00; // +0x00
    HxStr mUnknown08; // +0x08
    int mUnknown10;   // +0x10
    HxStr mUnknown14; // +0x14
    int mUnknown1c;   // +0x1c
    int mUnknown20;   // +0x20
    int mUnknown24;   // +0x24
    int mUnknown28;   // +0x28
    int mUnknown2c;   // +0x2c
    int mUnknown30;   // +0x30
};
