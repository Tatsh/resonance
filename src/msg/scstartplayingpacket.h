#pragma once

#include <iostream>

#include "msg/toallgamecontrollerspacket.h"

class IBStream;
class OBStream;

/**
 * Network packet the game sends between game systems.
 *
 * `20SCStartPlayingPacket` in the RTTI descriptor at `0x00902160`, with
 * ToAllGameControllersPacket as its one base. The object is 0x14 bytes and its vtable is at
 * `0x00814698`. The payload comes from the copy constructor at `0x003f3720`, which Clone()
 * delegates to, so the offsets and widths are recovered but the purpose of each field is not. The
 * four words Packet owns are declared there rather than here.
 *
 * The class adds no payload. Its allocation is exactly the 0x14 bytes Packet occupies, which is
 * what measures Packet.
 *
 * Slots 5, 6, and 7 of the table address three empty bodies of this class at `0x003f02a0`,
 * `0x003f0290`, and `0x003f0298`. The two transfer slots replace the non-empty Packet::Save() and
 * Packet::Load(). The packet therefore crosses the wire as zero bytes, without even the four
 * Packet words. The three bodies lie in the order Save(), Load(), Print() rather than in slot
 * order, the shape of three members defined in the class body in that order. Print() is
 * declared on that evidence alone, because an empty Print() at a unique address would otherwise
 * be indistinguishable from a re-emission of the empty Message::Print().
 *
 * The destructor at `0x003f01c0` is compiler-generated and has no declaration here.
 */
class SCStartPlayingPacket : public ToAllGameControllersPacket {
public:
    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nSCStartPlayingPacketType.
     *
     * @return The packet.
     * @ghidraAddress 0x003e4f40
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f01f8
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCStartPlayingPacketType.
     * @ghidraAddress 0x003f0270
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCStartPlayingPacket`.
     * @ghidraAddress 0x003f0280
     */
    virtual const char *Name();

    /**
     * Write nothing, replacing Packet::Save().
     *
     * Slot 6.
     *
     * @ghidraAddress 0x003f0290
     */
    virtual void Save(OBStream &) {
    }

    /**
     * Read nothing, replacing Packet::Load().
     *
     * Slot 7.
     *
     * @ghidraAddress 0x003f0298
     */
    virtual void Load(IBStream &) {
    }

    /**
     * Write nothing.
     *
     * Slot 5.
     *
     * @ghidraAddress 0x003f02a0
     */
    virtual void Print(std::ostream &) {
    }
};

/**
 * Identity that SCStartPlayingPacket::Type() reports.
 *
 * This word belongs to SCStartPlayingPacket because SCStartPlayingPacket::Type() at `0x003f0270`
 * returns it.
 *
 * @ghidraAddress 0x006d73ac
 */
extern int g_nSCStartPlayingPacketType;
