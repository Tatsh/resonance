#pragma once

#include <iostream>
#include <vector>

#include "game/freqappearance.h"
#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * A player's identity and settings.
 *
 * Its RTTI descriptor is at `0x0086f738`. It has no base. The compiler places the vptr after the
 * data at `+0x3c`, and the class is 0x40 bytes. Its vtable at `0x007d2820` has five entries and a
 * zero terminator at index 5, the type function, the destructor, and the three members below.
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
 * mOriginalId is the one member that none of Print(), Save(), or Load() touches. Only the
 * four-argument constructor writes it.
 *
 * Two network packets embed one of these, SCPlayerJoinedPacket and ToAllNetManagersPacket. The
 * image has no caller of the four-argument constructor. Every name below past the identifier,
 * colour, and appearance is inferred from its type and its initial value alone. The first three
 * follow the parameter order of Player's constructor and the `plid` and `clr` labels the join
 * packets print.
 */
class PlayerInfo {
public:
    /**
     * Construct an empty identity.
     *
     * Only the name, the appearance, and the vector are initialised. The integer members are left
     * as the storage holds them.
     *
     * Inline. The address is its uncalled out-of-line copy.
     *
     * @ghidraAddress NTSC-U/C: 0x00135d48
     * @ghidraAddress PAL: 0x001365d8
     */
    PlayerInfo() {
    }

    /**
     * Construct an identity from its parts.
     *
     * mActive starts at 1 and mReady at 0, and mOriginalId receives the same value as mPlayerId.
     *
     * Inline. The address is its uncalled out-of-line copy.
     *
     * @param nPlayerId The identifier stored in mPlayerId and mOriginalId.
     * @param colorName The colour name stored in mColorName.
     * @param appearance The appearance to copy.
     * @param nTrack The value stored in mTrack.
     * @ghidraAddress NTSC-U/C: 0x00135dd0
     * @ghidraAddress PAL: 0x00136678
     */
    PlayerInfo(unsigned nPlayerId,
               const HxStr &colorName,
               const FreqAppearance &appearance,
               int nTrack)
        : mPlayerId(nPlayerId), mColorName(colorName), mAppearance(appearance), mTrack(nTrack),
          mActive(1), mReady(0), mOriginalId(nPlayerId) {
    }

    /**
     * Release the vector, the appearance, and the name.
     *
     * @ghidraAddress NTSC-U/C: 0x00135e98
     * @ghidraAddress PAL: 0x00136750
     */
    virtual ~PlayerInfo();

    /**
     * Write the identity to a diagnostic stream.
     *
     * Slot 2. The output opens with the literal `{AppPlayerInfo: `, which is the one place in the
     * image that writes a name for this class outside its RTTI.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x00133930
     * @ghidraAddress PAL: 0x00134198
     */
    virtual void Print(std::ostream &stream) const;

    /**
     * Write the identity to a stream.
     *
     * Slot 3.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x00133578
     * @ghidraAddress PAL: 0x00133de0
     */
    virtual void Save(OBStream &stream) const;

    /**
     * Read the identity back from a stream.
     *
     * Slot 4. The members come back in the order Save() wrote them.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x00133740
     * @ghidraAddress PAL: 0x00133fa8
     */
    virtual void Load(IBStream &stream);

private:
    // Streamed through a different ostream overload than the int members below, and loaded and
    // stored as four bytes, which is what types it as unsigned rather than as int.
    unsigned mPlayerId;         // +0x00
    HxStr mColorName;           // +0x04
    FreqAppearance mAppearance; // +0x0c
    int mTrack;                 // +0x20
    int mActive;                // +0x24
    int mReady;                 // +0x28
    int mOriginalId;            // +0x2c
    std::vector<int> mEntries;  // +0x30 element type not recovered
};
