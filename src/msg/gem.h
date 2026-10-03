#pragma once

#include <iostream>

#include "mid/mbt.h"

class IBStream;
class OBStream;
class Player;

/**
 * Gem event a GemPacket reports.
 *
 * The five members form a subobject of GemPacket at `+0x14`, because the three routines below
 * receive `packet + 0x14` as their object and address it from zero. The class has no descriptor,
 * allocation tag, or literal. Its name comes from the debugging symbols of the North American demo
 * release. The demo's saveGuts() and Print() have the same instructions as this class's
 * saveGuts() and Print(). The name of each member is attested, from the label Print() writes
 * ahead of it.
 *
 * Every member is public, because the three routines are the only code in the image that refers
 * to the subobject and no accessor exists.
 */
struct Gem {
    /**
     * Write the five values to a stream.
     *
     * The player arrives on the wire as its identifier rather than as a pointer, and the value
     * written is mPlayer->mPlayerId.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x001a2560
     * @ghidraAddress PAL: 0x001a82c8
     */
    void saveGuts(OBStream &stream) const;

    /**
     * Read the five values back from a stream.
     *
     * Reads mGem, mTrans, and mBar, calls Mid::MBT::Load() for mLoc, and reads the identifier
     * into a local IDablePtr<Player>. Resolving it differs from IDablePtr's own conversion in one
     * respect. An identifier of -1 yields a null pointer here, while the conversion the packets'
     * Print() bodies expand would index the table at -1. The other two cases agree:
     * kIDableUnregistered yields NullPlayer::sInstance and any other value indexes the
     * IDable<Player> table.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x001a2630
     * @ghidraAddress PAL: 0x001a8398
     */
    void restoreGuts(IBStream &stream);

    /**
     * Write the five values to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x001a2ce0
     * @ghidraAddress PAL: 0x001a8a48
     */
    void Print(std::ostream &stream) const;

    int mGem;        /*!< Labelled `gem: `. +0x00 */
    int mTrans;      /*!< Labelled ` trans:`. +0x04 */
    int mBar;        /*!< Labelled ` bar:`. +0x08 */
    Mid::MBT mLoc;   /*!< Labelled ` loc:`. +0x0c */
    Player *mPlayer; /*!< Labelled ` pid:`, written as its identifier. +0x10 */
};
