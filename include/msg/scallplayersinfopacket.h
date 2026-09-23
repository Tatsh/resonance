#pragma once

#include <iostream>
#include <vector>

#include "msg/toallothernetmanagerspacket.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * `22SCAllPlayersInfoPacket` in the RTTI descriptor at `0x008f2a80`, with
 * ToAllOtherNetManagersPacket as its one base. The object is 0x20 bytes and its vtable is at
 * `0x008146e0`. The payload comes from the copy constructor at `0x003f34e8`, which Clone()
 * delegates to, and it accounts for the allocation exactly. The four words Packet provides are
 * declared there rather than here.
 *
 * The vector at `+0x14` is deep-copied, so the class owns its elements. Each element is 0x14 bytes,
 * a player identifier, an unlabelled word, and a vector of track numbers, which Print() labels
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
    struct Entry14 {
        int mPid;                 /*!< Labelled ` pid:`. +0x00 */
        int mUnknown04;           /*!< Transferred but not printed. +0x04 */
        std::vector<int> mTracks; /*!< Labelled ` tracks:`. +0x08 */
    };

    /**
     * Produce a packet with no entries on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nSCAllPlayersInfoPacketType.
     *
     * @return The packet.
     * @ghidraAddress 0x003e4ee8
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003eff60
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCAllPlayersInfoPacketType.
     * @ghidraAddress 0x003effd8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCAllPlayersInfoPacket`.
     * @ghidraAddress 0x003effe8
     */
    virtual const char *Name();

    /**
     * Write every entry to a diagnostic stream as ` pid:`, the identifier, ` tracks:`, the track
     * numbers in parentheses each followed by a space, and ` | `.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2278
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words, the entry count, and for every entry its two words, its track count,
     * and its tracks to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e6578
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the fields back in the order Save() wrote them, resizing both levels of vector to the
     * counts read.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e6788
     */
    virtual void Load(IBStream &stream);

    /**
     * Append a copy of one entry.
     *
     * The image lists no caller for the out-of-line body, and the verb is inferred from the body
     * alone.
     *
     * @param entry The entry to append.
     * @ghidraAddress 0x003f23a8
     */
    void AppendEntry(const Entry14 &entry);

private:
    std::vector<Entry14> mUnknown14; // +0x14
};

/**
 * Identity that SCAllPlayersInfoPacket::Type() reports.
 *
 * This word belongs to SCAllPlayersInfoPacket because SCAllPlayersInfoPacket::Type() at
 * `0x003effd8` returns it.
 *
 * @ghidraAddress 0x006d73a4
 */
extern int g_nSCAllPlayersInfoPacketType;
