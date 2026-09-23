#pragma once

#include <iostream>

#include "msg/toallothergamesystemspacket.h"

class IBStream;
class OBStream;
class Phrase;

/**
 * Network packet the game sends between game systems.
 *
 * `12PhrasePacket` in the RTTI descriptor at `0x00902130`, with ToAllOtherGameSystemsPacket as
 * its one base. The object is 0x20 bytes and its vtable is at `0x00814650`. The payload comes
 * from the copy constructor at `0x003f3760`, which Clone() delegates to. The four words Packet
 * provides are declared there rather than here.
 *
 * Print() labels `+0x14` as `tr` and `+0x1c` as `b`, and writes the first through the unsigned
 * integer inserter. The phrase at `+0x18` crosses the wire through the Phrase stream operators,
 * which allocate a fresh phrase on the reading side, and the packet never releases it.
 *
 * Save() writes the Packet word at `+0x0c` a second time after the payload, and Load() reads it a
 * second time to match.
 *
 * The destructor at `0x003f02a8` is compiler-generated and has no declaration here.
 */
class PhrasePacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nPhrasePacketType. The payload, the phrase pointer included, is left unset.
     *
     * @return The packet.
     * @ghidraAddress 0x003e51a8
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f0320
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nPhrasePacketType.
     * @ghidraAddress 0x003f0398
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `PhrasePacket`.
     * @ghidraAddress 0x003f03a8
     */
    virtual const char *Name();

    /**
     * Write `tr:`, the track, ` b:`, the bar, a space, and then the phrase, or `[empty]` without
     * one, to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2408
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the Packet words, the track, the bar, the phrase, and the Packet word at `+0x0c` again
     * to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e6b70
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the fields back in the order Save() wrote them.
     *
     * The phrase is replaced with a freshly allocated one without releasing the previous pointer.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e6ca8
     */
    virtual void Load(IBStream &stream);

private:
    unsigned int mTr; // +0x14
    Phrase *mPhrase;  // +0x18
    int mB;           // +0x1c
};

/**
 * Identity that PhrasePacket::Type() reports.
 *
 * This word belongs to PhrasePacket because PhrasePacket::Type() at `0x003f0398` returns it.
 *
 * @ghidraAddress 0x006d73d4
 */
extern int g_nPhrasePacketType;
