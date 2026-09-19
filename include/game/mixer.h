#pragma once

#include "app/msgsink.h"
#include "msg/message.h"

/**
 * Mixer for one track's voices.
 *
 * `5Mixer` in the RTTI descriptor at `0x008f0770`, deriving from MsgSink at offset 0, so the base
 * vptr lands at `+0x00` and this class's own members start at `+0x04`. Its table is at
 * `0x007df900` with four entries. The object is 0x58 bytes, which the tagged allocation in
 * `ScoreTrackGraph`'s constructor measures.
 *
 * The class is not reconstructed. Only the surface the stage classes use is declared, so that they
 * compile against the real type. Its constructor is at `0x001a7110` and takes the word
 * `ScoreTrackGraph` stores first along with the track's MIDI channel as a byte.
 */
class Mixer : public MsgSink {
public:
    /**
     * @ghidraAddress 0x001a8130
     */
    virtual ~Mixer();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001a7780
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Word the stage classes install through their table slot 6.
     *
     * The constructor does not write it, and all four stages write it from outside the class,
     * which is what records the member public. A friend declaration on ScoreTrackGraph fits the
     * image equally well.
     *
     * +0x04
     */
    int mUnknown04;
};
