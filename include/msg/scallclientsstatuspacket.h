#pragma once

#include <iostream>
#include <vector>

#include "msg/toallothernetmanagerspacket.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * `24SCAllClientsStatusPacket` in the RTTI descriptor at `0x008efe40`, with
 * ToAllOtherNetManagersPacket as its one base. The object is 0x20 bytes and its vtable is at
 * `0x00814770`. The payload comes from the copy constructor at `0x003f3298`, which Clone()
 * delegates to, and it accounts for the allocation exactly. The four words Packet owns are
 * declared there rather than here.
 *
 * The vector at `+0x14` is deep-copied, so the class owns its elements. Print() labels both words
 * of each element, which recovers their names.
 *
 * The destructor at `0x003efb88` is compiler-generated and has no declaration here.
 */
class SCAllClientsStatusPacket : public ToAllOtherNetManagersPacket {
public:
    /**
     * One client's status, eight bytes, from the stride the copy constructor divides by.
     *
     * Public because AppendEntry() takes one by value. The names are attested by the labels
     * `(id:` and ` stat:` that Print() writes ahead of the two words.
     */
    struct Entry14 {
        int mId;     /*!< Labelled `(id:`. +0x00 */
        int mStatus; /*!< Labelled ` stat:`. +0x04 */
    };

    /**
     * Produce a packet with no entries on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nSCAllClientsStatusPacketType.
     *
     * @return The packet.
     * @ghidraAddress 0x003e4e48
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003efaa8
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCAllClientsStatusPacketType.
     * @ghidraAddress 0x003efb20
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCAllClientsStatusPacket`.
     * @ghidraAddress 0x003efb30
     */
    virtual const char *Name();

    /**
     * Write every entry to a diagnostic stream as `(id:` id ` stat:` status `) `.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2160
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words, the entry count, and both words of every entry to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e6288
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the Packet words and the entry count, resize the vector, and read every entry in place.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e63f0
     */
    virtual void Load(IBStream &stream);

    /**
     * Append one entry.
     *
     * The entry arrives by value in two argument registers. The image lists no caller for the
     * out-of-line body, and the verb is inferred from the body alone.
     *
     * @param entry The entry to append.
     * @ghidraAddress 0x003f2218
     */
    void AppendEntry(Entry14 entry);

private:
    std::vector<Entry14> mUnknown14; // +0x14
};

/**
 * Identity that SCAllClientsStatusPacket::Type() reports.
 *
 * This word belongs to SCAllClientsStatusPacket because SCAllClientsStatusPacket::Type() at
 * `0x003efb20` returns it.
 *
 * @ghidraAddress 0x006d7394
 */
extern int g_nSCAllClientsStatusPacketType;
