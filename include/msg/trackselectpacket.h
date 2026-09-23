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
 * `17TrackSelectPacket` in the RTTI descriptor at `0x008ef620`, with ToAllOtherGameSystemsPacket
 * as its one base. The object is 0x28 bytes and its vtable is at `0x00814578`. The payload comes
 * from the copy constructor at `0x003f3868`, which Clone() delegates to. The four words Packet
 * provides are declared there rather than here.
 *
 * The member at `+0x14` is a Mid::MBT. Print() hands it to Mid::MBT::Print(), and New()
 * initialises it to kMBTInfinity. The transfer through the emission at `0x004acf28` alone could
 * not distinguish it from a CmdID. Print() writes the player at `+0x18` through Player::Print()
 * and labels `+0x20` as `track` and `+0x24` as `place`.
 *
 * The destructor at `0x003f07d0` is compiler-generated and has no declaration here.
 */
class TrackSelectPacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nTrackSelectPacketType.
     *
     * @return The packet.
     * @ghidraAddress 0x003e52c0
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f0848
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nTrackSelectPacketType.
     * @ghidraAddress 0x003f08c0
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `TrackSelectPacket`.
     * @ghidraAddress 0x003f08d0
     */
    virtual const char *Name();

    /**
     * Write the position, a space, the player, ` track:`, the track, ` place:`, and the place to a
     * diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2568
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words, the position, the player's identifier, the track, and the place to a
     * stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e71f8
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the fields back in place in the order Save() wrote them.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e7330
     */
    virtual void Load(IBStream &stream);

private:
    Mid::MBT mPosition;        // +0x14
    IDablePtr<Player> mPlayer; // +0x18
    int mTrack;                // +0x20
    int mPlace;                // +0x24
};

/**
 * Identity that TrackSelectPacket::Type() reports.
 *
 * This word belongs to TrackSelectPacket because TrackSelectPacket::Type() at `0x003f08c0`
 * returns it.
 *
 * @ghidraAddress 0x006d73ec
 */
extern int g_nTrackSelectPacketType;
