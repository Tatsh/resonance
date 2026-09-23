#pragma once

#include <iostream>

#include "msg/toallothernetmanagerspacket.h"
#include "os/hxstr.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * `14GameChatPacket` in the RTTI descriptor at `0x008efd70`, with ToAllOtherNetManagersPacket as
 * its one base. The object is 0x24 bytes and its vtable is at `0x00814338`. The payload comes
 * from the copy constructor at `0x003f3d88`, which Clone() delegates to, and it accounts for the
 * allocation exactly. The four words Packet owns are declared there rather than here.
 *
 * Each string crosses the wire as a four-byte length followed by that many bytes of text with no
 * terminator. Save() and Load() have second emissions at `0x003e8720` and `0x003e8890`.
 *
 * The destructor at `0x003f1898` is compiler-generated and has no declaration here.
 */
class GameChatPacket : public ToAllOtherNetManagersPacket {
public:
    /**
     * Produce a packet with two empty strings on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nGameChatPacketType.
     *
     * @return The packet.
     * @ghidraAddress 0x003e5478
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f1950
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nGameChatPacketType.
     * @ghidraAddress 0x003f19c8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `GameChatPacket`.
     * @ghidraAddress 0x003f19d8
     */
    virtual const char *Name();

    /**
     * Write both strings to a diagnostic stream, with nothing between them.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f28e8
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words and then both strings to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e8458
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the Packet words and then both strings back from a stream.
     *
     * Each string is resized through HxStr::Alloc() to the length read and then filled in place.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e85c8
     */
    virtual void Load(IBStream &stream);

private:
    HxStr mUnknown14; // +0x14
    HxStr mUnknown1c; // +0x1c
};

/**
 * Identity that GameChatPacket::Type() reports.
 *
 * This word belongs to GameChatPacket because GameChatPacket::Type() at `0x003f19c8` returns it.
 *
 * @ghidraAddress 0x006d740c
 */
extern int g_nGameChatPacketType;
