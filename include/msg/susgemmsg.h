#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `9SusGemMsg` in the RTTI descriptor at `0x00902270`, with Message as its one base. The object is
 * 0x1c bytes and its vtable is at `0x008124f8`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * A second, identical vtable at `0x007decf0` is emitted in the gem makers' unit, and
 * AxeNewGemMaker::PostGemMessages() stores it at `0x001a3094` and `0x001a3110`.
 *
 * The payload layout comes from the run of field copies in Clone(). Every member is public because
 * AppTunnel::HandleMessage() at `0x0044987c` reads them directly with no accessor in the image. It
 * copies the colour name of mPlayer, stops the strip mStripId when mStop is set, and otherwise
 * extends it on mLane to mFrame and mBlend.
 */
class SusGemMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 407.
     *
     * @return The message.
     * @ghidraAddress 0x003d7580
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001a44d0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nSusGemMsgType.
     * @ghidraAddress 0x001a4540
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `SusGemMsg`.
     * @ghidraAddress 0x001a4550
     */
    virtual const char *Name();

    int mStripId;    /*!< The sustain strip. +0x04 */
    int mStop;       /*!< Non-zero to stop the strip. +0x08 */
    int mLane;       /*!< The lane. +0x0c */
    int mFrame;      /*!< The frame the strip extends to. +0x10 */
    float mBlend;    /*!< The blend at that frame. +0x14 */
    Player *mPlayer; /*!< The player the strip belongs to. +0x18 */
};

/**
 * Identity that SusGemMsg::Type() reports.
 *
 * This word belongs to SusGemMsg because SusGemMsg::Type() at `0x001a4540` returns it, and the
 * registration at `0x003d9818` passes the same value, 407, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d02dc
 */
extern int g_nSusGemMsgType;
