#pragma once

#include <iostream>
#include <vector>

#include "msg/toallothernetmanagerspacket.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * Its RTTI descriptor is at `0x008efe40`. It has ToAllOtherNetManagersPacket as its one base. The
 * object is 0x20 bytes and its vtable is at `0x00814770`. The payload comes from the copy
 * constructor at `0x003f3298`. Clone() delegates to it, and the payload accounts for the
 * allocation exactly. The four words Packet provides are declared there rather than here.
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
    struct ClientStatus {
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
     * @ghidraAddress NTSC-U/C: 0x003e4e48
     * @ghidraAddress PAL: 0x0041d0e0
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003efaa8
     * @ghidraAddress PAL: 0x004280b0
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCAllClientsStatusPacketType.
     * @ghidraAddress NTSC-U/C: 0x003efb20
     * @ghidraAddress PAL: 0x00428128
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCAllClientsStatusPacket`.
     * @ghidraAddress NTSC-U/C: 0x003efb30
     * @ghidraAddress PAL: 0x00428138
     */
    virtual const char *Name();

    /**
     * Write every entry to a diagnostic stream as `(id:` id ` stat:` status `) `.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f2160
     * @ghidraAddress PAL: 0x0042a6a8
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words, the entry count, and both words of every entry to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e6288
     * @ghidraAddress PAL: 0x0041e568
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the Packet words and the entry count, resize the vector, and read every entry in place.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e63f0
     * @ghidraAddress PAL: 0x0041e6d0
     */
    virtual void Load(IBStream &stream);

    /**
     * Append one entry.
     *
     * The entry arrives by value in two argument registers. The image lists no caller for the
     * out-of-line body, and the verb is inferred from the body alone.
     *
     * @param entry The entry to append.
     * @ghidraAddress NTSC-U/C: 0x003f2218
     * @ghidraAddress PAL: 0x0042a760
     */
    void AppendEntry(ClientStatus entry);

private:
    std::vector<ClientStatus> mClients; // +0x14
};

/**
 * Identity that SCAllClientsStatusPacket::Type() reports.
 *
 * This word belongs to SCAllClientsStatusPacket because SCAllClientsStatusPacket::Type() at
 * `0x003efb20` returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d7394
 * @ghidraAddress PAL: 0x0071ab34
 */
extern int g_nSCAllClientsStatusPacketType;
