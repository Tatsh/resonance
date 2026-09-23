#pragma once

#include <iostream>

#include "game/idableptr.h"
#include "msg/toallothergamesystemspacket.h"

class IBStream;
class OBStream;
class Player;

/**
 * Network packet the game sends between game systems.
 *
 * `10BumpPacket` in the RTTI descriptor at `0x008ef700`, with ToAllOtherGameSystemsPacket as its
 * one base. The object is 0x28 bytes and its vtable is at `0x008144a0`. The payload comes from
 * the copy constructor at `0x003f3b58`, which Clone() delegates to. The four words Packet
 * provides are declared there rather than here.
 *
 * Print() labels `+0x1c` as a bar and `+0x20` as a track. mResult at `+0x24` is zeroed by New()
 * and is neither transferred nor printed.
 *
 * The destructor at `0x003f0e08` is compiler-generated and has no declaration here.
 */
class BumpPacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Construct a packet with only the player reference, the Packet words, and mResult set.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    BumpPacket() {
    }

    /**
     * Report a bumper deployed by a player.
     *
     * Inline, with no address of its own. BumpPowerup::Deploy() at `0x001c9de0` expands it on its
     * stack after the ToAllOtherGameSystemsPacket words.
     *
     * @param pPlayer The player who deployed the bumper, or null.
     * @param nBar The bar.
     * @param nTrack The track.
     */
    BumpPacket(Player *pPlayer, int nBar, int nTrack)
        : mPlayer(pPlayer), mBar(nBar), mTrack(nTrack) {
    }

    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nBumpPacketType.
     *
     * @return The packet.
     * @ghidraAddress 0x003e5410
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f0e80
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nBumpPacketType.
     * @ghidraAddress 0x003f0ef8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `BumpPacket`.
     * @ghidraAddress 0x003f0f08
     */
    virtual const char *Name();

    /**
     * Write the player's address, ` bar `, the bar, ` track `, and the track to a diagnostic
     * stream.
     *
     * The player is written through the pointer inserter, so the output is its address.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f26d8
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words, the player's identifier, the bar, and the track to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e7d88
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the Packet words, the player's identifier, the bar, and the track back in place.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e7eb0
     */
    virtual void Load(IBStream &stream);

private:
    IDablePtr<Player> mPlayer; // +0x14
    int mBar;                  // +0x1c
    int mTrack;                // +0x20

public:
    /**
     * Non-zero once a receiver has acted on the packet. +0x24
     *
     * Both constructors clear it. Public because BumpPowerup::Deploy() reads it back after sending,
     * plays `SND_DEPLOY_BUMPER` when it is set, and the image has no accessor.
     */
    int mResult = 0;
};

/**
 * Identity that BumpPacket::Type() reports.
 *
 * This word belongs to BumpPacket because BumpPacket::Type() at `0x003f0ef8` returns it.
 *
 * @ghidraAddress 0x006d7404
 */
extern int g_nBumpPacketType;
