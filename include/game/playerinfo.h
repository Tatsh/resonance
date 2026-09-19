#pragma once

#include <vector>

#include "game/freqappearance.h"
#include "os/hxstr.h"

/**
 * A player's identity and settings.
 *
 * `10PlayerInfo` in the RTTI descriptor at `0x0086f738`, with no base, so the compiler places the
 * vptr after the data at `+0x3c` and the class is 0x40 bytes. Its vtable is at `0x007d2820`.
 *
 * The layout comes from the copy constructor at `0x0010baa0`, which copies every member in order,
 * so the offsets are recovered and the HxStr and the embedded FreqAppearance are certain. The
 * vector at `+0x30` is deep-copied through the allocator and a memmove, which is what establishes
 * that the class owns its elements. The element type is not recovered. The purpose of each member
 * is not recovered, and no reader has been traced, so every member is private.
 *
 * Two network packets embed one of these, SCPlayerJoinedPacket and ToAllNetManagersPacket.
 */
class PlayerInfo {
public:
    /**
     * @param other The player to copy.
     * @ghidraAddress 0x0010baa0
     */
    PlayerInfo(const PlayerInfo &other);

private:
    int mUnknown00;              // +0x00
    HxStr mUnknown04;            // +0x04
    FreqAppearance mAppearance;  // +0x0c
    int mUnknown20;              // +0x20
    int mUnknown24;              // +0x24
    int mUnknown28;              // +0x28
    int mUnknown2c;              // +0x2c
    std::vector<int> mUnknown30; // +0x30 element type not recovered
};
