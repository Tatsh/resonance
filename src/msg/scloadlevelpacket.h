#pragma once

#include <iostream>

#include "game/gameparams.h"
#include "msg/toallgamecontrollerspacket.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * Its RTTI descriptor is at `0x008ef610`. It has ToAllGameControllersPacket as its one base. The
 * object is 0x4c bytes and its vtable is at `0x00814410`. The payload comes from the copy
 * constructor at `0x003f3c48`. Clone() delegates to it, and the payload accounts for the
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
     * @ghidraAddress NTSC-U/C: 0x003f1438
     * @ghidraAddress PAL: 0x00429900
     */
    SCLoadLevelPacket();

    /**
     * Construct a packet carrying a copy of some settings.
     *
     * The image lists no caller for the out-of-line body.
     *
     * @param params The settings to copy.
     * @ghidraAddress NTSC-U/C: 0x003f14b0
     * @ghidraAddress PAL: 0x00429978
     */
    SCLoadLevelPacket(const GameParams &params);

    /**
     * Produce a packet with default settings on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nSCLoadLevelPacketType.
     *
     * @return The packet.
     * @ghidraAddress NTSC-U/C: 0x003e5040
     * @ghidraAddress PAL: 0x0041d2d8
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003f13a0
     * @ghidraAddress PAL: 0x00429868
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCLoadLevelPacketType.
     * @ghidraAddress NTSC-U/C: 0x003f1418
     * @ghidraAddress PAL: 0x004298e0
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCLoadLevelPacket`.
     * @ghidraAddress NTSC-U/C: 0x003f1428
     * @ghidraAddress PAL: 0x004298f0
     */
    virtual const char *Name();

    /**
     * Write the settings to a diagnostic stream through GameParams::Print().
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f2858
     * @ghidraAddress PAL: 0x0042ada0
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words and the settings to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e8180
     * @ghidraAddress PAL: 0x00420460
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the Packet words and the settings back from a stream.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003f27a0
     * @ghidraAddress PAL: 0x0042ace8
     */
    virtual void Load(IBStream &stream);

    /**
     * Report the game settings.
     *
     * The image lists no caller. The title is inferred.
     *
     * @return A copy of the settings.
     * @ghidraAddress NTSC-U/C: 0x003f1528
     * @ghidraAddress PAL: 0x004299f0
     */
    GameParams GetParams();

private:
    GameParams mParams; // +0x14
};

/**
 * Identity that SCLoadLevelPacket::Type() reports.
 *
 * This word belongs to SCLoadLevelPacket because SCLoadLevelPacket::Type() at `0x003f1418`
 * returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d73bc
 * @ghidraAddress PAL: 0x0071ab5c
 */
extern int g_nSCLoadLevelPacketType;
