#pragma once

#include <iostream>

#include "game/idableptr.h"
#include "mid/mbt.h"
#include "msg/toallothergamesystemspacket.h"

class IBStream;
class OBStream;
class Player;

/**
 * Network packet the game sends between game systems.
 *
 * Its RTTI descriptor is at `0x00902060`. It has ToAllOtherGameSystemsPacket as its one base. The
 * object is 0x28 bytes and its vtable is at `0x00814530`. The payload comes from the copy
 * constructor at `0x003f38d0`. Clone() delegates to it. The four words Packet provides are
 * declared there rather than here.
 *
 * The member at `+0x1c` is a Mid::MBT. PrintExtra() hands it to Mid::MBT::Print() after the label
 * ` @`, and New() initialises it to kMBTInfinity. The transfer through the emission at
 * `0x004acf28` alone could not distinguish it from a CmdID. PrintExtra() labels `+0x20` as `track`
 * and `+0x24` as `succ`. The player reference at `+0x14` is transferred but not printed.
 *
 * Every member is public because Catcher::DispatchPriv() at `0x001adbec` reads the payload
 * directly with no accessor in the image.
 *
 * The destructor at `0x003f09f8` is compiler-generated and has no declaration here.
 */
class CatchProgressPacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nCatchProgressPacketType.
     *
     * @return The packet.
     * @ghidraAddress NTSC-U/C: 0x003e5330
     * @ghidraAddress PAL: 0x0041d5c8
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003f0a70
     * @ghidraAddress PAL: 0x00429078
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nCatchProgressPacketType.
     * @ghidraAddress NTSC-U/C: 0x003f0ae8
     * @ghidraAddress PAL: 0x004290f0
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `CatchProgressPacket`.
     * @ghidraAddress NTSC-U/C: 0x003f0af8
     * @ghidraAddress PAL: 0x00429100
     */
    virtual const char *GetName() const;

    /**
     * Write ` @`, the position, ` track:`, the track, ` succ:`, and the success value to a
     * diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f2648
     * @ghidraAddress PAL: 0x0042ab90
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Write the Packet words, the player's identifier, the position, the track, and the success
     * value to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e7430
     * @ghidraAddress PAL: 0x0041f710
     */
    virtual void saveGuts(OBStream &stream) const;

    /**
     * Read the fields back in place in the order saveGuts() wrote them.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e7568
     * @ghidraAddress PAL: 0x0041f848
     */
    virtual void restoreGuts(IBStream &stream);

    IDablePtr<Player> mPlayer; /*!< The catching player. +0x14 */
    Mid::MBT mPosition;        /*!< The song position, printed after ` @`. +0x1c */
    int mTrack;                /*!< Labelled `track` by PrintExtra(). +0x20 */
    float mSucc;               /*!< Labelled `succ` by PrintExtra(). +0x24 */
};

/**
 * Identity that CatchProgressPacket::Type() reports.
 *
 * This word belongs to CatchProgressPacket because CatchProgressPacket::Type() at `0x003f0ae8`
 * returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d73f4
 * @ghidraAddress PAL: 0x0071ab94
 */
extern int g_nCatchProgressPacketType;
