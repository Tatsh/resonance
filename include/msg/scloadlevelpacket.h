#pragma once

#include <iostream>

#include "game/gameparams.h"
#include "msg/toallgamecontrollerspacket.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * `17SCLoadLevelPacket` in the RTTI descriptor at `0x008ef610`, with ToAllGameControllersPacket
 * as its one base. The object is 0x4c bytes and its vtable is at `0x00814410`. The payload comes
 * from the copy constructor at `0x003f3c48`, which Clone() delegates to, and it accounts for the
 * allocation exactly. The four words Packet provides are declared there rather than here.
 *
 * Save() and Load() transfer the game settings through GameParams' own virtual Save() and Load()
 * after the Packet words.
 *
 * The destructor at `0x003f12c0` is compiler-generated and has no declaration here.
 */
class SCLoadLevelPacket : public ToAllGameControllersPacket {
public:
    /**
     * Construct a packet with default settings.
     *
     * The image lists no caller for the out-of-line body. New() expands the same stores in place.
     *
     * @ghidraAddress 0x003f1438
     */
    SCLoadLevelPacket();

    /**
     * Construct a packet carrying a copy of some settings.
     *
     * The image lists no caller for the out-of-line body.
     *
     * @param params The settings to copy.
     * @ghidraAddress 0x003f14b0
     */
    SCLoadLevelPacket(const GameParams &params);

    /**
     * Produce a packet with default settings on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nSCLoadLevelPacketType.
     *
     * @return The packet.
     * @ghidraAddress 0x003e5040
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f13a0
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCLoadLevelPacketType.
     * @ghidraAddress 0x003f1418
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCLoadLevelPacket`.
     * @ghidraAddress 0x003f1428
     */
    virtual const char *Name();

    /**
     * Write the settings to a diagnostic stream through GameParams::Print().
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2858
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words and the settings to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e8180
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the Packet words and the settings back from a stream.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003f27a0
     */
    virtual void Load(IBStream &stream);

    /**
     * Report the game settings.
     *
     * The image lists no caller. The title is inferred.
     *
     * @return A copy of the settings.
     * @ghidraAddress 0x003f1528
     */
    GameParams GetParams();

private:
    GameParams mUnknown14; // +0x14
};

/**
 * Identity that SCLoadLevelPacket::Type() reports.
 *
 * This word belongs to SCLoadLevelPacket because SCLoadLevelPacket::Type() at `0x003f1418`
 * returns it.
 *
 * @ghidraAddress 0x006d73bc
 */
extern int g_nSCLoadLevelPacketType;
