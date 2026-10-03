#pragma once

#include <iostream>

#include "msg/toarbiterpacket.h"
#include "os/hxstr.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * Its RTTI descriptor is at `0x00902070`. It has ToArbiterPacket as its one base. The object is
 * 0x24 bytes and its vtable is at `0x008142f0`. The payload comes from the copy constructor at
 * `0x003f3e58`. Clone() delegates to it, and the payload accounts for the allocation exactly. The
 * four words Packet provides are declared there rather than here.
 *
 * The layout matches GameChatPacket's, and Save() and Load() here are byte-identical to the
 * GameChatPacket routines. Slots 6 and 7 of this class's table address them. They are therefore
 * this class's members.
 *
 * The destructor at `0x003f1b40` is compiler-generated and has no declaration here.
 */
class TestArbiterPacket : public ToArbiterPacket {
public:
    /**
     * Construct a packet with two empty strings.
     *
     * Inline, with no address of its own. New() expands it.
     */
    TestArbiterPacket() {
    }

    /**
     * Construct a packet carrying two strings.
     *
     * The image lists no caller. The title is inferred.
     *
     * @param sender The string for mSender.
     * @param text The string for mText.
     * @ghidraAddress NTSC-U/C: 0x003f1cd0
     * @ghidraAddress PAL: 0x0042a208
     */
    TestArbiterPacket(const HxStr &sender, const HxStr &text);

    /**
     * Produce a packet with two empty strings on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nTestArbiterPacketType.
     *
     * @return The packet.
     * @ghidraAddress NTSC-U/C: 0x003e54d8
     * @ghidraAddress PAL: 0x0041d778
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003f1bf8
     * @ghidraAddress PAL: 0x0042a128
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nTestArbiterPacketType.
     * @ghidraAddress NTSC-U/C: 0x003f1c70
     * @ghidraAddress PAL: 0x0042a1a0
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `TestArbiterPacket`.
     * @ghidraAddress NTSC-U/C: 0x003f1c80
     * @ghidraAddress PAL: 0x0042a1b0
     */
    virtual const char *Name();

    /**
     * Write both strings to a diagnostic stream, with nothing between them.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f2ab8
     * @ghidraAddress PAL: 0x0042b000
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words and then both strings to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e8720
     * @ghidraAddress PAL: 0x00420a00
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the Packet words and then both strings back from a stream.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e8890
     * @ghidraAddress PAL: 0x00420b70
     */
    virtual void Load(IBStream &stream);

    /**
     * Report the first string.
     *
     * The image lists no caller. The title is inferred.
     *
     * @return A copy of mSender.
     * @ghidraAddress NTSC-U/C: 0x003f1d88
     * @ghidraAddress PAL: 0x0042a2d0
     */
    HxStr GetSender();

    /**
     * Report the second string.
     *
     * The image lists no caller. The title is inferred.
     *
     * @return A copy of mText.
     * @ghidraAddress NTSC-U/C: 0x003f1db8
     * @ghidraAddress PAL: 0x0042a300
     */
    HxStr GetText();

private:
    // Both titles follow GameChatPacket, whose layout and transfers this class repeats.
    HxStr mSender; // +0x14
    HxStr mText;   // +0x1c
};

/**
 * Identity that TestArbiterPacket::Type() reports.
 *
 * This word belongs to TestArbiterPacket because TestArbiterPacket::Type() at `0x003f1c70`
 * returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d7414
 * @ghidraAddress PAL: 0x0071abb4
 */
extern int g_nTestArbiterPacketType;
