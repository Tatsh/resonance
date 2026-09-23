#pragma once

#include "msg/message.h"
#include "os/hxstr.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `7TextMsg` in the RTTI descriptor at `0x008ef7b0`, with Message as its one base. The object is
 * 0x10 bytes and its vtable is at `0x007dc478`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). The HxStr at `+0x04` comes
 * from the copy constructor at `0x00193e10`, which copy-constructs it rather than copying its
 * words.
 *
 * mText is public because Overlay::OnText() at `0x0041f5e8` copies it directly at `0x0041f608`
 * with no accessor in the image. The purpose of the word at `+0x0c` is not recovered.
 */
class TextMsg : public Message {
public:
    /**
     * Construct a message with empty text.
     *
     * Inline. New() expands it, and it does not write the word at `+0x0c`.
     */
    TextMsg() {
    }

    /**
     * Construct a message showing a string.
     *
     * Inline. GrooveWorld::DisplayText() at `0x0018f140` expands it on its stack, copy-constructing
     * the text and clearing the word at `+0x0c`.
     *
     * @param text The text to show.
     */
    explicit TextMsg(const HxStr &text) : mText(text), mUnknown0c(0) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 311.
     *
     * @return The message.
     * @ghidraAddress 0x003d7118
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x00193e10
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nTextMsgType.
     * @ghidraAddress 0x00193eb8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `TextMsg`.
     * @ghidraAddress 0x00193ec8
     */
    virtual const char *Name();

    HxStr mText; /*!< The text the track display shows. +0x04 */

private:
    int mUnknown0c; // +0x0c
};

/**
 * Identity that TextMsg::Type() reports.
 *
 * This word belongs to TextMsg because TextMsg::Type() at `0x00193eb8` returns it, and the
 * registration at `0x003d9818` passes the same value, 311, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d0244
 */
extern int g_nTextMsgType;
