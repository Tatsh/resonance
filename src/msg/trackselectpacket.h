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
 * Its RTTI descriptor is at `0x008ef620`. It has ToAllOtherGameSystemsPacket as its one base. The
 * object is 0x28 bytes and its vtable is at `0x00814578`. The payload comes from the copy
 * constructor at `0x003f3868`. Clone() delegates to it. The four words Packet provides are
 * declared there rather than here.
 *
 * The member at `+0x14` is a Mid::MBT. PrintExtra() hands it to Mid::MBT::Print(), and New()
 * initialises it to kMBTInfinity. The transfer through the emission at `0x004acf28` alone could
 * not distinguish it from a CmdID. PrintExtra() writes the player at `+0x18` through
 * Player::Print() and labels `+0x20` as `track` and `+0x24` as `place`.
 *
 * The destructor at `0x003f07d0` is compiler-generated and has no declaration here.
 */
class TrackSelectPacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Construct a packet with the position at kMBTInfinity and no player.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    TrackSelectPacket() {
    }

    /**
     * Report a player's track selection to the other game systems.
     *
     * Inline, with no address of its own. LocalPlayer's track-select handler expands it on its
     * stack at `0x0011e9f8`.
     *
     * @param position The song position of the selection.
     * @param pPlayer The selecting player.
     * @param nTrack The selected track.
     * @param nPlace The player's place on the track.
     */
    TrackSelectPacket(Mid::MBT position, Player *pPlayer, int nTrack, int nPlace)
        : mPosition(position), mPlayer(pPlayer), mTrack(nTrack), mPlace(nPlace) {
    }

    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nTrackSelectPacketType.
     *
     * @return The packet.
     * @ghidraAddress NTSC-U/C: 0x003e52c0
     * @ghidraAddress PAL: 0x0041d558
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003f0848
     * @ghidraAddress PAL: 0x00428e50
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nTrackSelectPacketType.
     * @ghidraAddress NTSC-U/C: 0x003f08c0
     * @ghidraAddress PAL: 0x00428ec8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `TrackSelectPacket`.
     * @ghidraAddress NTSC-U/C: 0x003f08d0
     * @ghidraAddress PAL: 0x00428ed8
     */
    virtual const char *GetName() const;

    /**
     * Write the position, a space, the player, ` track:`, the track, ` place:`, and the place to a
     * diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f2568
     * @ghidraAddress PAL: 0x0042aab0
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Write the Packet words, the position, the player's identifier, the track, and the place to a
     * stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e71f8
     * @ghidraAddress PAL: 0x0041f4d8
     */
    virtual void saveGuts(OBStream &stream) const;

    /**
     * Read the fields back in place in the order saveGuts() wrote them.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e7330
     * @ghidraAddress PAL: 0x0041f610
     */
    virtual void restoreGuts(IBStream &stream);

public:
    /** The song position of the selection. NetPlayer's handler at `0x00122f78` reads it. +0x14 */
    Mid::MBT mPosition;

    /** The player that selected. NetPlayer's handler at `0x00122f78` resolves it. +0x18 */
    IDablePtr<Player> mPlayer;

    /** The selected track. NetPlayer's handler at `0x00122f78` reads it. +0x20 */
    int mTrack;

    /** The place on the track. NetPlayer's handler at `0x00122f78` reads it. +0x24 */
    int mPlace;
};

/**
 * Identity that TrackSelectPacket::Type() reports.
 *
 * This word belongs to TrackSelectPacket because TrackSelectPacket::Type() at `0x003f08c0`
 * returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d73ec
 * @ghidraAddress PAL: 0x0071ab8c
 */
extern int g_nTrackSelectPacketType;
