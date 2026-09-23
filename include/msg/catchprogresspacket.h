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
 * `19CatchProgressPacket` in the RTTI descriptor at `0x00902060`, with
 * ToAllOtherGameSystemsPacket as its one base. The object is 0x28 bytes and its vtable is at
 * `0x00814530`. The payload comes from the copy constructor at `0x003f38d0`, which Clone()
 * delegates to. The four words Packet provides are declared there rather than here.
 *
 * The member at `+0x1c` is a Mid::MBT. Print() hands it to Mid::MBT::Print() after the label ` @`,
 * and New() initialises it to kMBTInfinity. The transfer through the emission at `0x004acf28`
 * alone could not distinguish it from a CmdID. Print() labels `+0x20` as `track` and `+0x24` as
 * `succ`. The player reference at `+0x14` is transferred but not printed.
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
     * @ghidraAddress 0x003e5330
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f0a70
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nCatchProgressPacketType.
     * @ghidraAddress 0x003f0ae8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `CatchProgressPacket`.
     * @ghidraAddress 0x003f0af8
     */
    virtual const char *Name();

    /**
     * Write ` @`, the position, ` track:`, the track, ` succ:`, and the success value to a
     * diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2648
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words, the player's identifier, the position, the track, and the success
     * value to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e7430
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the fields back in place in the order Save() wrote them.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e7568
     */
    virtual void Load(IBStream &stream);

private:
    IDablePtr<Player> mPlayer; // +0x14
    Mid::MBT mPosition;        // +0x1c
    int mTrack;                // +0x20
    float mSucc;               // +0x24
};

/**
 * Identity that CatchProgressPacket::Type() reports.
 *
 * This word belongs to CatchProgressPacket because CatchProgressPacket::Type() at `0x003f0ae8`
 * returns it.
 *
 * @ghidraAddress 0x006d73f4
 */
extern int g_nCatchProgressPacketType;
