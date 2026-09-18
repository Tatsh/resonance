#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `9PhraseMsg` in the RTTI descriptor at `0x00901d30`, with Message as its one base. The object
 * is 0x10 bytes and its vtable is at `0x00811fe8`. The members below are the whole of the class:
 * everything recovered comes from them, and no other routine in the image refers to this type by
 * anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e42f8`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class PhraseMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e11e8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPhraseMsgType.
     * @ghidraAddress 0x003e1240
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PhraseMsg`.
     * @ghidraAddress 0x003e1250
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
};

/**
 * Identity that PhraseMsg::Type() reports.
 *
 * This word belongs to PhraseMsg because PhraseMsg::Type() at `0x003e1240` returns it. Several
 * handlers elsewhere read the same word to compare against it, which is the expected shape for a
 * registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d036c
 */
extern int g_nPhraseMsgType;
