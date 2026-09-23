#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12JamEffectMsg` in the RTTI descriptor at `0x008f07b0`, with Message as its one base. The
 * object is 0x14 bytes and its vtable is at `0x007e4528`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). Every field is public, because
 * JamEffectsMgr::PostRemixFxMsg() at `0x001a54d8` reads all four directly and the image has no
 * accessor for any of them.
 */
class JamEffectMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 417.
     *
     * @return The message.
     * @ghidraAddress 0x003d77e0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001caa00
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nJamEffectMsgType.
     * @ghidraAddress 0x001caa60
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `JamEffectMsg`.
     * @ghidraAddress 0x001caa70
     */
    virtual const char *Name();

    int mBar;    /*!< The bar the effect toggles on. +0x04 */
    int mTrack;  /*!< The track, which PostRemixFxMsg() compares with its manager's. +0x08 */
    int mEffect; /*!< The effect type, the bit PostRemixFxMsg() toggles in the bar's mask. +0x0c */
    /** Purpose unrecovered. PostRemixFxMsg() copies it into RemixFXMsg::mUnknown14. +0x10 */
    int mUnknown10;
};

/**
 * Identity that JamEffectMsg::Type() reports.
 *
 * This word belongs to JamEffectMsg because JamEffectMsg::Type() at `0x001caa60` returns it, and
 * the registration at `0x003d9818` passes the same value, 417, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d032c
 */
extern int g_nJamEffectMsgType;
