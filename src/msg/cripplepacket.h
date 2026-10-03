#pragma once

#include <iostream>
#include <vector>

#include "game/idableptr.h"
#include "msg/toallothergamesystemspacket.h"

class IBStream;
class OBStream;
class Player;

/**
 * Network packet the game sends between game systems.
 *
 * Its RTTI descriptor is at `0x008f0940`. It has ToAllOtherGameSystemsPacket as its one base. The
 * object is 0x28 bytes and its vtable is at `0x008144e8`. The payload comes from the copy
 * constructor at `0x003f3938`. Clone() delegates to it, and the payload accounts for the
 * allocation exactly. The four words Packet provides are declared there rather than here.
 *
 * The packet carries one player reference and a vector of further references. Load() resizes the
 * vector with a default reference of a null pointer and -1 as the fill value, which is the same
 * pair New() stores in the single reference and what identifies the element type. Print() writes
 * the players' addresses, not their identifiers.
 *
 * The destructor at `0x003ee2d8` is compiler-generated and has no declaration here.
 */
class CripplePacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Construct a packet with no references.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    CripplePacket() {
    }

    /**
     * Report a crippler deployed on a set of players.
     *
     * The body stores the attacker, then appends one reference per victim, taking the victims'
     * size again on every pass. The victims arrive by value, and the body releases their storage
     * on the way out. Gamer's crippler handler at `0x00111578` is the one caller.
     *
     * @param pAttacker The player who deployed the crippler.
     * @param victims The players it strikes.
     * @ghidraAddress NTSC-U/C: 0x003e7668
     * @ghidraAddress PAL: 0x0041f948
     */
    CripplePacket(Player *pAttacker, std::vector<Player *> victims);

    /**
     * Produce a packet with no references on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nCripplePacketType.
     *
     * @return The packet.
     * @ghidraAddress NTSC-U/C: 0x003e53a0
     * @ghidraAddress PAL: 0x0041d638
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003f0c60
     * @ghidraAddress PAL: 0x00429268
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nCripplePacketType.
     * @ghidraAddress NTSC-U/C: 0x003f0cd8
     * @ghidraAddress PAL: 0x004292e0
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `CripplePacket`.
     * @ghidraAddress NTSC-U/C: 0x003f0ce8
     * @ghidraAddress PAL: 0x004292f0
     */
    virtual const char *Name();

    /**
     * Write the first player's address and then every other player's address in parentheses,
     * each followed by a space, to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e7c38
     * @ghidraAddress PAL: 0x0041ff18
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words, the first player's identifier, the vector's count, and every other
     * player's identifier to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e7938
     * @ghidraAddress PAL: 0x0041fc18
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the Packet words, the first identifier, and the count, resize the vector, and read every
     * other identifier in place.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e7aa0
     * @ghidraAddress PAL: 0x0041fd80
     */
    virtual void Load(IBStream &stream);

private:
    IDablePtr<Player> mAttacker; // +0x14

public:
    /**
     * The players the crippler strikes. +0x1c
     *
     * Public because AppTunnel's crippler handler at `0x004486f8` walks it directly with no
     * accessor in the image.
     */
    std::vector<IDablePtr<Player> > mTargets;
};

/**
 * Identity that CripplePacket::Type() reports.
 *
 * This word belongs to CripplePacket because CripplePacket::Type() at `0x003f0cd8` returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d73fc
 * @ghidraAddress PAL: 0x0071ab9c
 */
extern int g_nCripplePacketType;
