#pragma once

#include <iostream>

#include "msg/toarbiterpacket.h"
#include "os/hxstr.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * `17TestArbiterPacket` in the RTTI descriptor at `0x00902070`, with ToArbiterPacket as its one
 * base. The object is 0x24 bytes and its vtable is at `0x008142f0`. The payload comes from the
 * copy constructor at `0x003f3e58`, which Clone() delegates to, and it accounts for the
 * allocation exactly. The four words Packet owns are declared there rather than here.
 *
 * The layout matches GameChatPacket's, and Save() and Load() here are byte-identical to that
 * class's. The program had titled both as copies of GameChatPacket's, but slots 6 and 7 of this
 * class's table address them, so they are this class's members.
 *
 * The destructor at `0x003f1b40` is compiler-generated and has no declaration here.
 */
class TestArbiterPacket : public ToArbiterPacket {
public:
    /**
     * Produce a packet with two empty strings on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nTestArbiterPacketType.
     *
     * @return The packet.
     * @ghidraAddress 0x003e54d8
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f1bf8
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nTestArbiterPacketType.
     * @ghidraAddress 0x003f1c70
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `TestArbiterPacket`.
     * @ghidraAddress 0x003f1c80
     */
    virtual const char *Name();

    /**
     * Write both strings to a diagnostic stream, with nothing between them.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2ab8
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words and then both strings to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e8720
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the Packet words and then both strings back from a stream.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e8890
     */
    virtual void Load(IBStream &stream);

private:
    HxStr mUnknown14; // +0x14
    HxStr mUnknown1c; // +0x1c
};

/**
 * Identity that TestArbiterPacket::Type() reports.
 *
 * This word belongs to TestArbiterPacket because TestArbiterPacket::Type() at `0x003f1c70`
 * returns it.
 *
 * @ghidraAddress 0x006d7414
 */
extern int g_nTestArbiterPacketType;
