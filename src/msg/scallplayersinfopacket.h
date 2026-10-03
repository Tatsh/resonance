#pragma once

#include <iostream>
#include <vector>

#include "msg/toallothernetmanagerspacket.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * Its RTTI descriptor is at `0x008f2a80`. It has ToAllOtherNetManagersPacket as its one base. The
 * object is 0x20 bytes and its vtable is at `0x008146e0`. The payload comes from the copy
 * constructor at `0x003f34e8`. Clone() delegates to it, and the payload accounts for the
 * allocation exactly. The four words Packet provides are declared there rather than here.
 *
 * The vector at `+0x14` is deep-copied, so the class owns its elements. Each element is 0x14 bytes,
 * a player identifier, an unlabelled word, and a vector of track numbers, which PrintExtra() labels
 * ` pid:` and ` tracks:`.
 *
 * The destructor at `0x003f0040` is compiler-generated and has no declaration here.
 */
class SCAllPlayersInfoPacket : public ToAllOtherNetManagersPacket {
public:
    /**
     * One player's entry, from the 0x14-byte stride the copy constructor divides by.
     *
     * Public because AppendEntry() takes one. The copy constructor at `0x0010a2e0` is the
     * compiler-generated one.
     */
    struct PlayerEntry {
        int mPid; /*!< Labelled ` pid:`. +0x00 */
        /**
         * Transferred but not printed. +0x04
         *
         * The title is inferred from the `pid` and `clid` pair GemPacket prints.
         */
        int mClientId;
        std::vector<int> mTracks; /*!< Labelled ` tracks:`. +0x08 */
    };

    /**
     * Produce a packet with no entries on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nSCAllPlayersInfoPacketType.
     *
     * @return The packet.
     * @ghidraAddress NTSC-U/C: 0x003e4ee8
     * @ghidraAddress PAL: 0x0041d180
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003eff60
     * @ghidraAddress PAL: 0x00428568
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCAllPlayersInfoPacketType.
     * @ghidraAddress NTSC-U/C: 0x003effd8
     * @ghidraAddress PAL: 0x004285e0
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCAllPlayersInfoPacket`.
     * @ghidraAddress NTSC-U/C: 0x003effe8
     * @ghidraAddress PAL: 0x004285f0
     */
    virtual const char *GetName() const;

    /**
     * Write every entry to a diagnostic stream as ` pid:`, the identifier, ` tracks:`, the track
     * numbers in parentheses each followed by a space, and ` | `.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f2278
     * @ghidraAddress PAL: 0x0042a7c0
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Write the Packet words, the entry count, and for every entry its two words, its track count,
     * and its tracks to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e6578
     * @ghidraAddress PAL: 0x0041e858
     */
    virtual void saveGuts(OBStream &stream) const;

    /**
     * Read the fields back in the order saveGuts() wrote them, resizing both levels of vector to
     * the counts read.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e6788
     * @ghidraAddress PAL: 0x0041ea68
     */
    virtual void restoreGuts(IBStream &stream);

    /**
     * Append a copy of one entry.
     *
     * The image lists no caller for the out-of-line body, and the verb is inferred from the body
     * alone.
     *
     * @param entry The entry to append.
     * @ghidraAddress NTSC-U/C: 0x003f23a8
     * @ghidraAddress PAL: 0x0042a8f0
     */
    void AppendEntry(const PlayerEntry &entry);

private:
    std::vector<PlayerEntry> mPlayers; // +0x14
};

/**
 * Identity that SCAllPlayersInfoPacket::Type() reports.
 *
 * This word belongs to SCAllPlayersInfoPacket because SCAllPlayersInfoPacket::Type() at
 * `0x003effd8` returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d73a4
 * @ghidraAddress PAL: 0x0071ab44
 */
extern int g_nSCAllPlayersInfoPacketType;
