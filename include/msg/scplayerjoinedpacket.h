#pragma once

#include <iostream>

#include "game/playerinfo.h"
#include "msg/toallnetmanagerspacket.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * `20SCPlayerJoinedPacket` in the RTTI descriptor at `0x00902050`, with ToAllNetManagersPacket as
 * its one base. The object is 0x54 bytes and its vtable is at `0x00814800`. The payload comes
 * from the copy constructor at `0x003f31c8`, which Clone() delegates to, and it accounts for the
 * allocation exactly. The four words Packet provides are declared there rather than here.
 *
 * The payload is one PlayerInfo, and Save(), Load(), and Print() hand it to PlayerInfo's own
 * virtual Save(), Load(), and Print() after the Packet words.
 *
 * The destructor at `0x003ed8f8` is compiler-generated and has no declaration here.
 */
class SCPlayerJoinedPacket : public ToAllNetManagersPacket {
public:
    /**
     * Construct a packet with a default player record.
     *
     * The image lists no caller for the out-of-line body. New() expands the same stores in place.
     *
     * @ghidraAddress 0x003ef7b0
     */
    SCPlayerJoinedPacket();

    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nSCPlayerJoinedPacketType.
     *
     * @return The packet.
     * @ghidraAddress 0x003e4cf8
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003ef718
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCPlayerJoinedPacketType.
     * @ghidraAddress 0x003ef790
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCPlayerJoinedPacket`.
     * @ghidraAddress 0x003ef7a0
     */
    virtual const char *Name();

    /**
     * Write the player record to a diagnostic stream through PlayerInfo::Print().
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2100
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words and the player record to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e5fb8
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the Packet words and the player record back from a stream.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003f2048
     */
    virtual void Load(IBStream &stream);

private:
    PlayerInfo mUnknown14; // +0x14
};

/**
 * Identity that SCPlayerJoinedPacket::Type() reports.
 *
 * This word belongs to SCPlayerJoinedPacket because SCPlayerJoinedPacket::Type() at `0x003ef790`
 * returns it.
 *
 * @ghidraAddress 0x006d7384
 */
extern int g_nSCPlayerJoinedPacketType;
