#pragma once

#include <iostream.h>
#include <vector>

#include "game/freqappearance.h"
#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * A player's identity and settings.
 *
 * `10PlayerInfo` in the RTTI descriptor at `0x0086f738`, with no base, so the compiler places the
 * vptr after the data at `+0x3c` and the class is 0x40 bytes. Its vtable at `0x007d2820` has five
 * entries and a zero terminator at index 5, the type function, the destructor, and the three
 * members below.
 *
 * The layout comes from the copy constructor at `0x0010baa0` and the destructor together. The copy
 * constructor copies every member in order, and the destructor releases the vector, the appearance,
 * and the string buffer in reverse declaration order. The vector at `+0x30` is deep-copied through
 * the allocator and a memmove, which is what establishes that the class owns its elements. The
 * element type is not recovered, and `int` stands in for it. Every member is private, because the
 * only readers outside the class are the three members below.
 *
 * That copy constructor is the compiler-generated one and is therefore not declared here. It copies
 * all eight members in exact declaration order with no logic of its own, and it sits at
 * `0x0010baa0` while every hand-written member of the class sits in the `0x00133xxx` and
 * `0x00135xxx` runs, which places it in a different translation unit.
 *
 * mUnknown2c is the one member that none of Print(), Save(), or Load() touches.
 *
 * Two network packets embed one of these, SCPlayerJoinedPacket and ToAllNetManagersPacket.
 */
class PlayerInfo {
public:
    /**
     * Release the vector, the appearance, and the name.
     *
     * @ghidraAddress 0x00135e98
     */
    virtual ~PlayerInfo();

    /**
     * Write the identity to a diagnostic stream.
     *
     * Slot 2. The output opens with the literal `{AppPlayerInfo: `, which is the one place in the
     * image that writes a name for this class outside its RTTI.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00133930
     */
    virtual void Print(ostream &stream);

    /**
     * Write the identity to a stream.
     *
     * Slot 3.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00133578
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the identity back from a stream.
     *
     * Slot 4. The members come back in the order Save() wrote them.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00133740
     */
    virtual void Load(IBStream &stream);

private:
    // Streamed through a different ostream overload than the int members below, and loaded and
    // stored as four bytes, which is what types it as unsigned rather than as int.
    unsigned mUnknown00;         // +0x00
    HxStr mUnknown04;            // +0x04
    FreqAppearance mAppearance;  // +0x0c
    int mUnknown20;              // +0x20
    int mUnknown24;              // +0x24
    int mUnknown28;              // +0x28
    int mUnknown2c;              // +0x2c
    std::vector<int> mUnknown30; // +0x30 element type not recovered
};
