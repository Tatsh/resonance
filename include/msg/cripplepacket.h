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
 * `13CripplePacket` in the RTTI descriptor at `0x008f0940`, with ToAllOtherGameSystemsPacket as
 * its one base. The object is 0x28 bytes and its vtable is at `0x008144e8`. The payload comes
 * from the copy constructor at `0x003f3938`, which Clone() delegates to, and it accounts for the
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
     * Produce a packet with no references on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nCripplePacketType.
     *
     * @return The packet.
     * @ghidraAddress 0x003e53a0
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f0c60
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nCripplePacketType.
     * @ghidraAddress 0x003f0cd8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `CripplePacket`.
     * @ghidraAddress 0x003f0ce8
     */
    virtual const char *Name();

    /**
     * Write the first player's address and then every other player's address in parentheses,
     * each followed by a space, to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e7c38
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words, the first player's identifier, the vector's count, and every other
     * player's identifier to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e7938
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the Packet words, the first identifier, and the count, resize the vector, and read every
     * other identifier in place.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e7aa0
     */
    virtual void Load(IBStream &stream);

private:
    IDablePtr<Player> mUnknown14;               // +0x14
    std::vector<IDablePtr<Player> > mUnknown1c; // +0x1c
};

/**
 * Identity that CripplePacket::Type() reports.
 *
 * This word belongs to CripplePacket because CripplePacket::Type() at `0x003f0cd8` returns it.
 *
 * @ghidraAddress 0x006d73fc
 */
extern int g_nCripplePacketType;
