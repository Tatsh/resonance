#pragma once

#include <iostream>
#include <vector>

#include "game/freqappearance.h"
#include "game/gameparams.h"
#include "game/playerinfo.h"
#include "msg/tosinglenetmanagerpacket.h"
#include "os/hxstr.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * Its RTTI descriptor is at `0x008ef110`. It has ToSingleNetManagerPacket as its one base. The
 * object is 0x7c bytes and its vtable is at `0x00814890`. The payload comes from the copy
 * constructor at `0x003f2e48`. Clone() delegates to it, and the payload accounts for the
 * allocation exactly. The four words Packet provides are declared there rather than here.
 *
 * Print() labels the words at `+0x18` and `+0x14` as `plid` and `destid`, the string at `+0x54` as
 * `clr`, and the appearance at `+0x5c` as `thm`. The vector at `+0x70` holds PlayerInfo records.
 * Save() and Load() step through it 0x40 bytes at a time and dispatch each element's Save() and
 * Load() through the PlayerInfo vtable pointer at the element's `+0x3c`, and Load() resizes it with
 * a default-constructed PlayerInfo as the fill value.
 *
 * The destructor at `0x003ed5e0` is compiler-generated and has no declaration here.
 */
class SPJoinAcceptPacket : public ToSingleNetManagerPacket {
public:
    /**
     * Construct a packet with default settings, an empty colour, a fresh appearance, and no
     * players.
     *
     * The image lists no caller for the out-of-line body. New() expands the same stores in place.
     *
     * @ghidraAddress NTSC-U/C: 0x003ef110
     * @ghidraAddress PAL: 0x00427660
     */
    SPJoinAcceptPacket();

    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nSPJoinAcceptPacketType.
     *
     * @return The packet.
     * @ghidraAddress NTSC-U/C: 0x003e4b40
     * @ghidraAddress PAL: 0x0041cd70
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003ef078
     * @ghidraAddress PAL: 0x004275c8
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSPJoinAcceptPacketType.
     * @ghidraAddress NTSC-U/C: 0x003ef0f0
     * @ghidraAddress PAL: 0x00427640
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SPJoinAcceptPacket`.
     * @ghidraAddress NTSC-U/C: 0x003ef100
     * @ghidraAddress PAL: 0x00427650
     */
    virtual const char *Name();

    /**
     * Write ` plid:`, ` destid:`, the settings, ` clr:`, and ` thm:` with their values to a
     * diagnostic stream.
     *
     * The player records are not written.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f1f58
     * @ghidraAddress PAL: 0x0042a4a0
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words, both identifiers, the settings, the colour, the appearance, the
     * record count, and every record to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e5718
     * @ghidraAddress PAL: 0x0041d9c0
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the fields back in the order Save() wrote them, resizing the record vector to the count
     * read.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e5930
     * @ghidraAddress PAL: 0x0041dbd8
     */
    virtual void Load(IBStream &stream);

    /**
     * Report the username of the carried appearance.
     *
     * The image lists no caller. The title is inferred.
     *
     * @return A copy of the appearance's first string.
     * @ghidraAddress NTSC-U/C: 0x003ef390
     * @ghidraAddress PAL: 0x00427968
     */
    HxStr GetUsername();

    /**
     * Report the game settings.
     *
     * The image lists no caller. The title is inferred.
     *
     * @return A copy of the settings.
     * @ghidraAddress NTSC-U/C: 0x003ef3c0
     * @ghidraAddress PAL: 0x00427998
     */
    GameParams GetParams();

    /**
     * Report the colour name.
     *
     * The image lists no caller. The title is inferred.
     *
     * @return A copy of mColorName.
     * @ghidraAddress NTSC-U/C: 0x003ef3f0
     * @ghidraAddress PAL: 0x004279c8
     */
    HxStr GetColorName();

    /**
     * Report the carried appearance.
     *
     * The image lists no caller. The title is inferred.
     *
     * @return A copy of the appearance.
     * @ghidraAddress NTSC-U/C: 0x003ef420
     * @ghidraAddress PAL: 0x004279f8
     */
    FreqAppearance GetAppearance();

    /**
     * Append one player to the player list.
     *
     * The image lists no caller. The title is inferred.
     *
     * @param player The player to copy in.
     * @ghidraAddress NTSC-U/C: 0x003ef450
     * @ghidraAddress PAL: 0x00427a28
     */
    void AddPlayer(const PlayerInfo &player);

private:
    int mDestId;                      // +0x14
    int mPlayerId;                    // +0x18
    GameParams mParams;               // +0x1c
    HxStr mColorName;                 // +0x54
    FreqAppearance mAppearance;       // +0x5c
    std::vector<PlayerInfo> mPlayers; // +0x70
};

/**
 * Identity that SPJoinAcceptPacket::Type() reports.
 *
 * This word belongs to SPJoinAcceptPacket because SPJoinAcceptPacket::Type() at `0x003ef0f0`
 * returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d7374
 * @ghidraAddress PAL: 0x0071ab14
 */
extern int g_nSPJoinAcceptPacketType;
