#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `8PitchMsg` in the RTTI descriptor at `0x00901af0`, with Message as its one base. The object is
 * 0x14 bytes and its vtable is at `0x007e5560`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). The last three words are
 * public because AppTunnel's pitch handler at `0x00447cc0` reads them directly with no accessor in
 * the image. It compares `+0x10` with the player each tunnel item stores, which types it.
 *
 * The destructor at `0x001b3890` is compiler-generated and has no declaration here.
 */
class PitchMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 409.
     *
     * @return The message.
     * @ghidraAddress 0x003d7600
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001b3940
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPitchMsgType.
     * @ghidraAddress 0x001b39a0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PitchMsg`.
     * @ghidraAddress 0x001b39b0
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04

public:
    int mUnknown08;     /*!< Purpose unrecovered. AppTunnel passes it on to a lookup. +0x08 */
    int mUnknown0c;     /*!< Purpose unrecovered. AppTunnel converts it to a float. +0x0c */
    Player *mUnknown10; /*!< The player. +0x10 */
};

/**
 * Identity that PitchMsg::Type() reports.
 *
 * This word belongs to PitchMsg because PitchMsg::Type() at `0x001b39a0` returns it, and the
 * registration at `0x003d9818` passes the same value, 409, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d02ec
 */
extern int g_nPitchMsgType;
