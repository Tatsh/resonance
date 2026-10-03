#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"

/**
 * Sink that delivers everything it receives to every registered sink.
 *
 * Its RTTI descriptor is at `0x008f0800`. It has MsgSink at offset 0 and MsgSource at offset 4. The
 * object is 0x18 bytes: the MsgSink vptr at `+0x00`, the MsgSource subobject over `+0x04` through
 * `+0x17`, and the MsgSource vptr inside that at `+0x14`. The class declares no data member.
 *
 * Its primary table is at `0x007e0128` and its MsgSource table at `0x007e0100`. MuseSynth embeds
 * one at `+0x0c` and exposes it as the sink every player it creates sends to.
 *
 * The three bodies sit inside MuseSynth's run of code, so they are written in that translation
 * unit. The name comes from the RTTI descriptor.
 */
class MsgSplitter : public MsgSink, public MsgSource {
public:
    /**
     * @ghidraAddress NTSC-U/C: 0x001aae68
     * @ghidraAddress PAL: 0x001b0bd0
     */
    MsgSplitter();

    /**
     * @ghidraAddress NTSC-U/C: 0x001aad98
     * @ghidraAddress PAL: 0x001b0b00
     */
    virtual ~MsgSplitter();

    /**
     * Deliver a message to every registered sink.
     *
     * Primary table slot 3. Forwards to MsgSource::Send().
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x001ab4a8
     * @ghidraAddress PAL: 0x001b1210
     */
    virtual void DispatchPriv(Message *pMsg);
};
