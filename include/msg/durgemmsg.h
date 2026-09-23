#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `9DurGemMsg` in the RTTI descriptor at `0x00901ce0`, with Message as its one base. The object is
 * 0x20 bytes and its vtable is at `0x007ded38`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). Every member but the word at
 * `+0x18` is public because AppTunnel::HandleMessage() at `0x004497e0` reads them directly with no
 * accessor in the image. It copies the colour name of mPlayer and draws the gem from mLane between
 * the two frame and blend pairs, converting each frame to a float.
 */
class DurGemMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 406.
     *
     * @return The message.
     * @ghidraAddress 0x003d7538
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001a4388
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nDurGemMsgType.
     * @ghidraAddress 0x001a4400
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `DurGemMsg`.
     * @ghidraAddress 0x001a4410
     */
    virtual const char *Name();

    int mLane;         /*!< The lane. +0x04 */
    int mStartFrame;   /*!< The frame the gem starts at. +0x08 */
    float mStartBlend; /*!< The blend at the start. +0x0c */
    int mEndFrame;     /*!< The frame the gem ends at. +0x10 */
    float mEndBlend;   /*!< The blend at the end. +0x14 */

private:
    int mUnknown18; // +0x18

public:
    Player *mPlayer; /*!< The player the gem belongs to. +0x1c */
};

/**
 * Identity that DurGemMsg::Type() reports.
 *
 * This word belongs to DurGemMsg because DurGemMsg::Type() at `0x001a4400` returns it, and the
 * registration at `0x003d9818` passes the same value, 406, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d02d4
 */
extern int g_nDurGemMsgType;
