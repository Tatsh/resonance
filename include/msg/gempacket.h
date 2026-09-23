#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/toallothergamesystemspacket.h"

class IBStream;
class OBStream;
class Player;

/**
 * Network packet the game sends between game systems.
 *
 * `9GemPacket` in the RTTI descriptor at `0x008ef710`, with ToAllOtherGameSystemsPacket as its
 * one base. The object is 0x2c bytes and its vtable is at `0x00814380`, with eight entries and a
 * zero terminator at index 8. Slot 1 is the compiler-generated destructor, slots 2 through 4
 * supply the three pure slots Packet leaves open, and slots 5, 6, and 7 override Message::Print(),
 * Packet::Save(), and Packet::Load(). The four words Packet provides are declared there rather
 * than here.
 *
 * This class shares its RTTI accessor and vtable with ToAllOtherGameSystemsPacket, its own base,
 * which has no implementation of its own. The vtable belongs to this class.
 *
 * The payload layout is recovered from the three field routines rather than from the copy
 * constructor. The copy constructor at `0x003f3d18` moves `+0x14` through `+0x23` with two
 * unaligned 64-bit pairs, which reads as two eight-byte members and is the compiler merging
 * adjacent four-byte fields. Save() at `0x001a2560`, Load() at `0x001a2630`, and Print() at
 * `0x001a2ce0` transfer the same region as five separate four-byte lvalues, and Print() labels
 * each one, which is what recovers both the widths and the names.
 *
 * The allocation tag on both the allocation in Clone() and the release in the destructor is `MSG`,
 * which is the tag Message declares rather than one of this class.
 */
class GemPacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Gem event the packet reports.
     *
     * The five members form a subobject rather than part of the packet, because the three
     * routines below receive `packet + 0x14` as their object and address it from zero. `Fields` is
     * a placeholder for the name, which no descriptor, allocation tag, or literal in the image
     * supplies. The name of each member is attested, from the label Print() writes ahead of it.
     *
     * Every member is public, because the three routines are the only code in the image that
     * refers to the subobject and no accessor exists.
     */
    struct Fields {
        /**
         * Write the five values to a stream.
         *
         * The player arrives on the wire as its identifier rather than as a pointer, and the
         * value written is mPlayer->mId20.
         *
         * @param stream The stream to write to.
         * @ghidraAddress 0x001a2560
         */
        void Save(OBStream &stream);

        /**
         * Read the five values back from a stream.
         *
         * Reads mGem, mTrans, and mBar, calls Mid::MBT::Load() for mLoc, and reads the identifier
         * into a local IDablePtr<Player>. Resolving it differs from IDablePtr's own conversion in
         * one respect. An identifier of -1 yields a null pointer here, while the conversion the
         * packets' Print() bodies expand would index the table at -1. The other two cases agree:
         * kIDableUnregistered yields g_nullPlayer and any other value indexes the IDable<Player>
         * table.
         *
         * @param stream The stream to read from.
         * @ghidraAddress 0x001a2630
         */
        void Load(IBStream &stream);

        /**
         * Write the five values to a diagnostic stream.
         *
         * @param stream The stream to write to.
         * @ghidraAddress 0x001a2ce0
         */
        void Print(std::ostream &stream);

        int mGem;        /*!< Labelled `gem: `. +0x00 */
        int mTrans;      /*!< Labelled ` trans:`. +0x04 */
        int mBar;        /*!< Labelled ` bar:`. +0x08 */
        Mid::MBT mLoc;   /*!< Labelled ` loc:`. +0x0c */
        Player *mPlayer; /*!< Labelled ` pid:`, written as its identifier. +0x10 */
    };

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
     * stack. That routine fills a local Fields with mGem, mBar, mLoc, and mPlayer, leaving mTrans
     * unset, copies the whole subobject in, and passes the manager's word at `+0x30` as the track.
     *
     * @param fields The gem event.
     * @param nTr The track.
     */
    GemPacket(const Fields &fields, int nTr) : mFields(fields), mTr(nTr) {
    }

    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nGemPacketType.
     *
     * @return The packet.
     * @ghidraAddress 0x003e5148
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f1750
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nGemPacketType.
     * @ghidraAddress 0x003f17c8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `GemPacket`.
     * @ghidraAddress 0x003f17d8
     */
    virtual const char *Name();

    /**
     * Write a description of the packet to a diagnostic stream.
     *
     * Slot 5, overriding Message::Print().
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2878
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the packet to a stream.
     *
     * Slot 6, overriding Packet::Save().
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e8258
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the packet back from a stream.
     *
     * Slot 7, overriding Packet::Load().
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e8368
     */
    virtual void Load(IBStream &stream);

private:
    Fields mFields; // +0x14
    // Labelled ` tr:` by Print(), which is the only recovered evidence of its purpose.
    int mTr; // +0x28
};

/**
 * Identity that GemPacket::Type() reports.
 *
 * This word belongs to GemPacket because GemPacket::Type() at `0x003f17c8` returns it.
 *
 * @ghidraAddress 0x006d73cc
 */
extern int g_nGemPacketType;
