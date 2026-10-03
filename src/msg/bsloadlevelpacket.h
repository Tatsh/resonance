#pragma once

#include <iostream>

#include "game/gameparams.h"
#include "msg/tohostpacket.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * Its RTTI descriptor is at `0x008ef600`. It has ToHostPacket as its one base. The object is 0x4c
 * bytes and its vtable is at `0x00814458`. The payload comes from the copy constructor at
 * `0x003f3bc0`. Clone() delegates to it, and the payload accounts for the allocation exactly. The
 * four words Packet provides are declared there rather than here.
 *
 * saveGuts() and restoreGuts() transfer the game settings through GameParams' own virtual Save()
 * and Load() and then transfer the Packet word at `+0x0c` a second time.
 *
 * The destructor at `0x003f1038` is compiler-generated and has no declaration here.
 */
class BSLoadLevelPacket : public ToHostPacket {
public:
    /**
     * Construct a packet with default settings.
     *
     * The image lists no caller for the out-of-line body. New() expands the same stores in place.
     *
     * @ghidraAddress NTSC-U/C: 0x003f11b0
     * @ghidraAddress PAL: 0x00429718
     */
    BSLoadLevelPacket();

    /**
     * Construct a packet carrying a copy of some settings.
     *
     * After the copy the Packet word at `+0x0c` is set to zero rather than to the -1 every other
     * construction leaves there. The image lists no caller for the out-of-line body.
     *
     * @param params The settings to copy.
     * @ghidraAddress NTSC-U/C: 0x003f1220
     * @ghidraAddress PAL: 0x00429788
     */
    BSLoadLevelPacket(const GameParams &params);

    /**
     * Produce a packet with default settings on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nBSLoadLevelPacketType.
     *
     * @return The packet.
     * @ghidraAddress NTSC-U/C: 0x003e4f98
     * @ghidraAddress PAL: 0x0041d230
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003f1118
     * @ghidraAddress PAL: 0x00429680
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nBSLoadLevelPacketType.
     * @ghidraAddress NTSC-U/C: 0x003f1190
     * @ghidraAddress PAL: 0x004296f8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `BSLoadLevelPacket`.
     * @ghidraAddress NTSC-U/C: 0x003f11a0
     * @ghidraAddress PAL: 0x00429708
     */
    virtual const char *GetName() const;

    /**
     * Write the settings to a diagnostic stream through GameParams::Print().
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f2780
     * @ghidraAddress PAL: 0x0042acc8
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Write the Packet words, the settings, and the Packet word at `+0x0c` again to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e7fa0
     * @ghidraAddress PAL: 0x00420280
     */
    virtual void saveGuts(OBStream &stream) const;

    /**
     * Read the fields back in the order saveGuts() wrote them.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e80a0
     * @ghidraAddress PAL: 0x00420380
     */
    virtual void restoreGuts(IBStream &stream);

    /**
     * Report the game settings.
     *
     * The image lists no caller. The title is inferred.
     *
     * @return A copy of the settings.
     * @ghidraAddress NTSC-U/C: 0x003f1290
     * @ghidraAddress PAL: 0x004297f8
     */
    GameParams GetParams();

private:
    GameParams mParams; // +0x14
};

/**
 * Identity that BSLoadLevelPacket::Type() reports.
 *
 * This word belongs to BSLoadLevelPacket because BSLoadLevelPacket::Type() at `0x003f1190`
 * returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d73b4
 * @ghidraAddress PAL: 0x0071ab54
 */
extern int g_nBSLoadLevelPacketType;
