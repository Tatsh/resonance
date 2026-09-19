#pragma once

#include "msg/musemsg.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12MultiMuseMsg` in the RTTI descriptor at `0x008eec98`, with MuseMsg as its one base. The object
 * is 0xc bytes and its vtable is at `0x00812df8`. Clone() delegates to the copy constructor at
 * `0x003e38a8` rather than copying inline, which is why this class is recovered alongside the
 * packets rather than with its three MuseMsg siblings.
 *
 * The word at `+0x08` is what settles MuseMsg's payload. This class copies a word there while
 * NoteMsg and StdMidiMsg copy bytes, and a single word store cannot straddle a base and a derived
 * class in a member-wise copy, so `+0x08` belongs to each derived class rather than to MuseMsg.
 *
 * The class overrides Message::Print() at `0x003e3990`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class MultiMuseMsg : public MuseMsg {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dc590
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_dwMultiMuseMsgType.
     * @ghidraAddress 0x003dc608
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `MultiMuseMsg`.
     * @ghidraAddress 0x003dc618
     */
    virtual const char *Name();

private:
    int mUnknown08; // +0x08
};

/**
 * Identity that MultiMuseMsg::Type() reports.
 *
 * This word belongs to MultiMuseMsg because MultiMuseMsg::Type() at `0x003dc608` returns it. The
 * program still titles it `g_dwMsgIdAddLightPoint`, which describes a handler comparing against it
 * rather than the class reporting it, so the two disagree until that label is corrected.
 *
 * @ghidraAddress 0x006d01dc
 */
extern unsigned int g_dwMultiMuseMsgType;
