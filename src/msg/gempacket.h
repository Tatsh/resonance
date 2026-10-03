#pragma once

#include <iostream>

#include "msg/gem.h"
#include "msg/toallothergamesystemspacket.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * Its RTTI descriptor is at `0x008ef710`. It has ToAllOtherGameSystemsPacket as its one base. The
 * object is 0x2c bytes and its vtable is at `0x00814380`, with eight entries and a zero terminator
 * at index 8. Slot 1 is the compiler-generated destructor, slots 2 through 4 supply the three pure
 * slots Packet does not implement, and slots 5, 6, and 7 override Message::PrintExtra(),
 * Packet::saveGuts(), and Packet::restoreGuts(). The four words Packet provides are declared there
 * rather than here.
 *
 * This class shares its RTTI accessor and vtable with ToAllOtherGameSystemsPacket, its own base,
 * which has no implementation of its own. The vtable belongs to this class.
 *
 * The payload layout is recovered from the three field routines rather than from the copy
 * constructor. The copy constructor at `0x003f3d18` moves `+0x14` through `+0x23` with two
 * unaligned 64-bit pairs, which reads as two eight-byte members and is the compiler merging
 * adjacent four-byte fields. Gem::saveGuts() at `0x001a2560`, Gem::restoreGuts() at
 * `0x001a2630`, and Gem::Print() at `0x001a2ce0` transfer the same region as five separate
 * four-byte lvalues, and Gem::Print() labels each one, which is what recovers both the widths and
 * the names.
 *
 * The allocation tag on both the allocation in Clone() and the release in the destructor is `MSG`,
 * which is the tag Message declares rather than one of this class.
 *
 * The destructor at `0x003f16d8` is compiler-generated and has no declaration here.
 */
class GemPacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Construct a packet with the payload unset beyond the Packet words and the position.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    GemPacket() {
    }

    /**
     * Report one gem event.
     *
     * Inline, with no address of its own. PhraseMgr::AddGem() at `0x001bab84` expands it on its
     * stack. That routine fills a local Gem with mGem, mBar, mLoc, and mPlayer, leaving mTrans
     * unset, copies the whole subobject in, and passes the manager's word at `+0x30` as the track.
     *
     * @param fields The gem event.
     * @param nTr The track.
     */
    GemPacket(const Gem &fields, int nTr) : mFields(fields), mTr(nTr) {
    }

    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nGemPacketType.
     *
     * @return The packet.
     * @ghidraAddress NTSC-U/C: 0x003e5148
     * @ghidraAddress PAL: 0x0041d3e0
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003f1750
     * @ghidraAddress PAL: 0x00429c18
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nGemPacketType.
     * @ghidraAddress NTSC-U/C: 0x003f17c8
     * @ghidraAddress PAL: 0x00429c90
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `GemPacket`.
     * @ghidraAddress NTSC-U/C: 0x003f17d8
     * @ghidraAddress PAL: 0x00429ca0
     */
    virtual const char *GetName() const;

    /**
     * Write a description of the packet to a diagnostic stream.
     *
     * Slot 5, overriding Message::PrintExtra().
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f2878
     * @ghidraAddress PAL: 0x0042adc0
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Write the packet to a stream.
     *
     * Slot 6, overriding Packet::saveGuts().
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e8258
     * @ghidraAddress PAL: 0x00420538
     */
    virtual void saveGuts(OBStream &stream) const;

    /**
     * Read the packet back from a stream.
     *
     * Slot 7, overriding Packet::restoreGuts().
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e8368
     * @ghidraAddress PAL: 0x00420648
     */
    virtual void restoreGuts(IBStream &stream);

    /**
     * The gem. +0x14
     *
     * PhraseMgr::PostGemMsg() at `0x001ba6d0` reads it directly.
     */
    Gem mFields;

    /**
     * The track, labelled ` tr:` by PrintExtra(). +0x28
     *
     * PhraseMgr::PostGemMsg() compares it with the track the manager serves.
     */
    int mTr;
};

/**
 * Identity that GemPacket::Type() reports.
 *
 * This word belongs to GemPacket because GemPacket::Type() at `0x003f17c8` returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d73cc
 * @ghidraAddress PAL: 0x0071ab6c
 */
extern int g_nGemPacketType;
