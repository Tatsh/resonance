#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"

/**
 * Sink that delivers everything it receives to every registered sink.
 *
 * `11MsgSplitter` in the RTTI descriptor at `0x008f0800`, with MsgSink at offset 0 and MsgSource
 * at offset 4. The object is 0x18 bytes: the MsgSink vptr at `+0x00`, the MsgSource subobject over
 * `+0x04` through `+0x17`, and the MsgSource vptr inside that at `+0x14`. The class declares no
 * data member of its own.
 *
 * Its primary table is at `0x007e0128` and its MsgSource table at `0x007e0100`. MuseSynth embeds
 * one at `+0x0c` and exposes it as the sink every player it creates sends to.
 *
 * The three bodies sit inside MuseSynth's run of code, so they are written in that translation
 * unit. The name comes from the RTTI descriptor and is not the invented `Rnd::LightMsgSplitter`
 * that the type-function harvest recorded for it.
 */
class MsgSplitter : public MsgSink, public MsgSource {
public:
    /**
     * @ghidraAddress 0x001aae68
     */
    MsgSplitter();

    /**
     * @ghidraAddress 0x001aad98
     */
    virtual ~MsgSplitter();

    /**
     * Deliver a message to every registered sink.
     *
     * Primary table slot 3. Forwards to MsgSource::Send().
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001ab4a8
     */
    virtual void HandleMessage(Message *pMsg);
};
