#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "msg/message.h"

/**
 * Manager of the phrases one track is divided into.
 *
 * `9PhraseMgr` in the RTTI descriptor at `0x008ef9d0`, over MsgSink at offset 0 and MsgSource at
 * offset 4. The primary table is at `0x007e28d0` and the MsgSource subobject table at
 * `0x007e28a8`. The object is 0x60 bytes, which the tagged allocation in `ScoreTrackGraph`'s
 * constructor measures.
 *
 * Its title is attested by the mangled signature of `Catcher::Catcher()`,
 * `__7CatcherP9PhraseMgrP9QuantizerPC9TrackDataPQ23Sch9TickClockiGQ23Sch4Tick`, where
 * `P9PhraseMgr` is the first parameter. Every stage passes the one it retains to the catcher it
 * builds.
 *
 * The class is not reconstructed. Only the surface the stage classes use is declared, so that they
 * compile against the real type. Its own routines are titled in the program at `0x001ba0d0` for
 * the constructor, `0x001bafa8` for PostCaughtPhrasePacket(), `0x001ba750` for PostGemMsg(), and
 * `0x001bbb20` for PostBarStatusMsg().
 */
class PhraseMgr : public MsgSink, public MsgSource {
public:
    /**
     * @ghidraAddress 0x001ba2b8
     */
    virtual ~PhraseMgr();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001bc718
     */
    virtual void HandleMessage(Message *pMsg);

private:
    int mUnknown18; // +0x18

public:
    /**
     * Word the stage classes install through their table slot 8.
     *
     * The constructor clears it and every one of the four stages writes it from outside the class,
     * which is what records the member public. A friend declaration on ScoreTrackGraph fits the
     * image equally well.
     *
     * +0x1c
     */
    int mUnknown1c;
};
