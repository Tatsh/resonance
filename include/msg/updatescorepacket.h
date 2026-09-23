#pragma once

#include <iostream>

#include "msg/toallothergamesystemspacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `17UpdateScorePacket` in the RTTI descriptor at `0x008efcc0`, with ToAllOtherGameSystemsPacket
 * as its one base. The object is 0x1c bytes and its vtable is at `0x008145c0`. The payload comes
 * from the copy constructor at `0x003f3818`, which Clone() delegates to, so the offsets and
 * widths are recovered but the purpose of each field is not. The four words Packet owns are
 * declared there rather than here.
 *
 * Its vtable has eight entries and a zero terminator at index 8. Slots 5, 6, and 7 are all its own
 * overrides. Both transfer members open by expanding the Packet pair inline rather than calling it,
 * which every one of the twenty overriding packet classes does identically.
 *
 * Print() labels the two members `pid:` and ` score-delta:`. The builds in two Player routines at
 * `0x0012f904` and `0x0012fa6c` store the player's identifier from its `+0x20` and the amount
 * added, which settles both names.
 */
class UpdateScorePacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Construct a packet with only the Packet words set.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    UpdateScorePacket() {
    }

    /**
     * Report a change to a player's score.
     *
     * Inline, with no address of its own. The two Player routines at `0x0012f808` and
     * `0x0012f970` expand it on their stacks after the ToAllOtherGameSystemsPacket words.
     *
     * @param nPlayerId The player's identifier.
     * @param nScoreDelta The amount added to the score.
     */
    UpdateScorePacket(int nPlayerId, int nScoreDelta)
        : mPlayerId(nPlayerId), mScoreDelta(nScoreDelta) {
    }

    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nUpdateScorePacketType. Only the Packet words are initialised.
     *
     * @return The packet.
     * @ghidraAddress 0x003e5268
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f06c0
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nUpdateScorePacketType.
     * @ghidraAddress 0x003f0738
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `UpdateScorePacket`.
     * @ghidraAddress 0x003f0748
     */
    virtual const char *Name();

    /**
     * Write the payload to a diagnostic stream.
     *
     * Slot 5. The literals are at `0x00814240` and `0x00814248`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2510
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the packet to a stream.
     *
     * Slot 6.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e7018
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the packet back from a stream.
     *
     * Slot 7. Both members are filled in place rather than through a local.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e7120
     */
    virtual void Load(IBStream &stream);

public:
    /** The identifier of the scoring player. Player::HandleMessage() reads it. +0x14 */
    int mPlayerId;

    /** The points to add. Player::HandleMessage() reads it. +0x18 */
    int mScoreDelta;
};

/**
 * Identity that UpdateScorePacket::Type() reports.
 *
 * This word belongs to UpdateScorePacket because UpdateScorePacket::Type() at `0x003f0738`
 * returns it.
 *
 * @ghidraAddress 0x006d73e4
 */
extern int g_nUpdateScorePacketType;
