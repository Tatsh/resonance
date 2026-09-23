#pragma once

#include <iostream>

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `17PhraseCapturedMsg` in the RTTI descriptor at `0x008ef830`, with Message as its one base. The
 * object is 0x28 bytes and its vtable is at `0x008126f0`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). Print() labels five of the
 * nine words: a bar range at `+0x04` and `+0x08`, the track at `+0x14`, the score at `+0x1c`, and
 * the juice at `+0x20`, and it writes the colour name of the player at `+0x18`. The words at
 * `+0x0c`, `+0x10`, and `+0x24` are not printed. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The destructor at `0x003deaf0` is compiler-generated and has no declaration here.
 */
class PhraseCapturedMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress 0x003d73d0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003debe0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPhraseCapturedMsgType.
     * @ghidraAddress 0x003dec68
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PhraseCapturedMsg`.
     * @ghidraAddress 0x003dec78
     */
    virtual const char *Name();

    /**
     * Write `b `, the bar range joined by `--`, ` tr# `, the track, ` score `, the score,
     * ` juice `, the juice, a space, and the player's colour name to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003d8448
     */
    virtual void Print(std::ostream &stream);

private:
    int mUnknown04;  // +0x04
    int mUnknown08;  // +0x08
    int mUnknown0c;  // +0x0c
    int mUnknown10;  // +0x10
    int mTrack;      // +0x14
    Player *mPlayer; // +0x18
    int mScore;      // +0x1c
    int mJuice;      // +0x20
    int mUnknown24;  // +0x24
};

/**
 * Identity that PhraseCapturedMsg::Type() reports.
 *
 * This word belongs to PhraseCapturedMsg because PhraseCapturedMsg::Type() at `0x003dec68`
 * returns it. Several handlers elsewhere read the same word to compare against it, which is the
 * expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d02a4
 */
extern int g_nPhraseCapturedMsgType;
