#pragma once

#include "msg/tohostpacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * Its RTTI descriptor is at `0x00902120`. It has ToHostPacket as its one base. The object is 0x14
 * bytes and its vtable is at `0x00814728`. The payload comes from the copy constructor at
 * `0x003f34a8`. Clone() delegates to it. The offsets and widths are recovered, but the purpose of
 * each field is not. The four words Packet provides are declared there rather than here.
 *
 * The class adds no payload. Its allocation is exactly the 0x14 bytes Packet occupies, which is
 * what measures Packet.
 *
 * The destructor at `0x003efc70` is compiler-generated and has no declaration here.
 */
class CSInitiatePlayPacket : public ToHostPacket {
public:
    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nCSInitiatePlayPacketType.
     *
     * @return The packet.
     * @ghidraAddress NTSC-U/C: 0x003e4ea0
     * @ghidraAddress PAL: 0x0041d138
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003efca8
     * @ghidraAddress PAL: 0x004282b0
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nCSInitiatePlayPacketType.
     * @ghidraAddress NTSC-U/C: 0x003efd20
     * @ghidraAddress PAL: 0x00428328
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `CSInitiatePlayPacket`.
     * @ghidraAddress NTSC-U/C: 0x003efd30
     * @ghidraAddress PAL: 0x00428338
     */
    virtual const char *GetName() const;
};

/**
 * Identity that CSInitiatePlayPacket::Type() reports.
 *
 * This word belongs to CSInitiatePlayPacket because CSInitiatePlayPacket::Type() at `0x003efd20`
 * returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d739c
 * @ghidraAddress PAL: 0x0071ab3c
 */
extern int g_nCSInitiatePlayPacketType;
