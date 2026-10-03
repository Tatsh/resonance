#pragma once

#include <iostream>

#include "msg/tosinglenetmanagerpacket.h"
#include "os/hxstr.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * Its RTTI descriptor is at `0x008efe30`. It has ToSingleNetManagerPacket as its one base. The
 * object is 0x20 bytes and its vtable is at `0x00814848`. The payload comes from the copy
 * constructor at `0x003f3130`. Clone() delegates to it. The offsets and widths are recovered, but
 * the purpose of each field is not. The four words Packet provides are declared there rather than
 * here.
 *
 * This class shares its RTTI accessor and vtable with ToSingleNetManagerPacket, its own base,
 * which has no implementation of its own. The vtable belongs to this class.
 *
 * The destructor at `0x003ef4b8` is compiler-generated and has no declaration here.
 */
class SPJoinDenyPacket : public ToSingleNetManagerPacket {
public:
    /**
     * Construct a packet with an empty string.
     *
     * Inline. New() expands it, zeroing only the string, and no out-of-line copy exists. A
     * declaration is required because the class declares a second constructor.
     */
    SPJoinDenyPacket() {
    }

    /**
     * Construct a packet from a word and a string, which is copied.
     *
     * The image lists no caller for the out-of-line body.
     *
     * @param nReasonCode The word stored at `+0x14`.
     * @param reason The string copied into `+0x18`.
     * @ghidraAddress NTSC-U/C: 0x003ef628
     * @ghidraAddress PAL: 0x00427c18
     */
    SPJoinDenyPacket(int nReasonCode, const HxStr &reason);

    /**
     * Produce a packet with an empty string on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nSPJoinDenyPacketType. The word at `+0x14` is left unset.
     *
     * @return The packet.
     * @ghidraAddress NTSC-U/C: 0x003e4ca0
     * @ghidraAddress PAL: 0x0041cf18
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003ef558
     * @ghidraAddress PAL: 0x00427b40
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSPJoinDenyPacketType.
     * @ghidraAddress NTSC-U/C: 0x003ef5d0
     * @ghidraAddress PAL: 0x00427bb8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SPJoinDenyPacket`.
     * @ghidraAddress NTSC-U/C: 0x003ef5e0
     * @ghidraAddress PAL: 0x00427bc8
     */
    virtual const char *GetName() const;

    /**
     * Write the word as a number, a space, and the string to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f2000
     * @ghidraAddress PAL: 0x0042a548
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Write the Packet words, the word, and the string to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e5d58
     * @ghidraAddress PAL: 0x0041e038
     */
    virtual void saveGuts(OBStream &stream) const;

    /**
     * Read the Packet words, the word, and the string back from a stream.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e5e98
     * @ghidraAddress PAL: 0x0041e178
     */
    virtual void restoreGuts(IBStream &stream);

private:
    // Both titles are inferred from the class name, a refusal of a join request.
    int mReasonCode; // +0x14
    HxStr mReason;   // +0x18
};

/**
 * Identity that SPJoinDenyPacket::Type() reports.
 *
 * This word belongs to SPJoinDenyPacket because SPJoinDenyPacket::Type() at `0x003ef5d0` returns
 * it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d737c
 * @ghidraAddress PAL: 0x0071ab1c
 */
extern int g_nSPJoinDenyPacketType;
