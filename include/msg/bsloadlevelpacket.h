#pragma once

#include <iostream>

#include "game/gameparams.h"
#include "msg/tohostpacket.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * `17BSLoadLevelPacket` in the RTTI descriptor at `0x008ef600`, with ToHostPacket as its one
 * base. The object is 0x4c bytes and its vtable is at `0x00814458`. The payload comes from the
 * copy constructor at `0x003f3bc0`, which Clone() delegates to, and it accounts for the
 * allocation exactly. The four words Packet provides are declared there rather than here.
 *
 * Save() and Load() transfer the game settings through GameParams' own virtual Save() and Load()
 * and then transfer the Packet word at `+0x0c` a second time.
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
     * @ghidraAddress 0x003f11b0
     */
    BSLoadLevelPacket();

    /**
     * Construct a packet carrying a copy of some settings.
     *
     * After the copy the Packet word at `+0x0c` is set to zero rather than to the -1 every other
     * construction leaves there. The image lists no caller for the out-of-line body.
     *
     * @param params The settings to copy.
     * @ghidraAddress 0x003f1220
     */
    BSLoadLevelPacket(const GameParams &params);

    /**
     * Produce a packet with default settings on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nBSLoadLevelPacketType.
     *
     * @return The packet.
     * @ghidraAddress 0x003e4f98
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f1118
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nBSLoadLevelPacketType.
     * @ghidraAddress 0x003f1190
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `BSLoadLevelPacket`.
     * @ghidraAddress 0x003f11a0
     */
    virtual const char *Name();

    /**
     * Write the settings to a diagnostic stream through GameParams::Print().
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2780
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words, the settings, and the Packet word at `+0x0c` again to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e7fa0
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the fields back in the order Save() wrote them.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e80a0
     */
    virtual void Load(IBStream &stream);

    /**
     * Report the game settings.
     *
     * The image lists no caller. The title is inferred.
     *
     * @return A copy of the settings.
     * @ghidraAddress 0x003f1290
     */
    GameParams GetParams();

private:
    GameParams mUnknown14; // +0x14
};

/**
 * Identity that BSLoadLevelPacket::Type() reports.
 *
 * This word belongs to BSLoadLevelPacket because BSLoadLevelPacket::Type() at `0x003f1190`
 * returns it.
 *
 * @ghidraAddress 0x006d73b4
 */
extern int g_nBSLoadLevelPacketType;
