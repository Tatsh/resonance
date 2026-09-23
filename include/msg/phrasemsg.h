#pragma once

#include <iostream>

#include "msg/message.h"

class Phrase;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `9PhraseMsg` in the RTTI descriptor at `0x00901d30`, with Message as its one base. The object
 * is 0x10 bytes and its vtable is at `0x00811fe8`. The members below are the whole of the class:
 * everything recovered comes from them, and no other routine in the image refers to this type by
 * anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). PhraseMgr::PostPhraseMsg()
 * fills the three words with its argument, a manager word, and the phrase it looked up, which
 * types `+0x0c`. Print() writes that phrase as an address followed by the word at `+0x04` in
 * brackets. Readers of the fields have not been traced, so they are private by default.
 *
 * The destructor at `0x003e10f8` is compiler-generated and has no declaration here.
 */
class PhraseMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress 0x003d79b0
     */
    static Message *New();

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

    /**
     * Write the phrase's address, ` [`, the word at `+0x04`, and `]` to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e42f8
     */
    virtual void Print(std::ostream &stream);

    /**
     * The bar of the phrase. +0x04
     *
     * AxeOldGemMaker::OnPhrase() at `0x001a3600` multiplies it by the 1920 ticks of a bar.
     */
    int mBar;

private:
    int mUnknown08; // +0x08

public:
    /**
     * The phrase. +0x0c
     *
     * AxeOldGemMaker::OnPhrase() at `0x001a3600` replays its sequence.
     */
    Phrase *mPhrase;
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
