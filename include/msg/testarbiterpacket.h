#pragma once

#include "msg/toarbiterpacket.h"
#include "os/hxstr.h"

/**
 * Network packet the game sends between game systems.
 *
 * `17TestArbiterPacket` in the RTTI descriptor at `0x00902070`, with ToArbiterPacket as its one
 * base. The object is 0x24 bytes and its vtable is at `0x008142f0`. The payload comes from the
 * copy constructor at `0x003f3e58`, which Clone() delegates to, and it accounts for the
 * allocation exactly. The four words Packet owns are declared there rather than here.
 *
 * The class overrides Message::Print() at `0x003f2ab8`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class TestArbiterPacket : public ToArbiterPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f1bf8
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nTestArbiterPacketType.
     * @ghidraAddress 0x003f1c70
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `TestArbiterPacket`.
     * @ghidraAddress 0x003f1c80
     */
    virtual const char *Name();

private:
    HxStr mUnknown14; // +0x14
    HxStr mUnknown1c; // +0x1c
};

/**
 * Identity that TestArbiterPacket::Type() reports.
 *
 * This word belongs to TestArbiterPacket because TestArbiterPacket::Type() at `0x003f1c70`
 * returns it.
 *
 * @ghidraAddress 0x006d7414
 */
extern int g_nTestArbiterPacketType;
