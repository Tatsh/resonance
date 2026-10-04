#pragma once

#include <vector>

#include "app/msgsink.h"
#include "msg/message.h"
#include "msg/stdmidimsg.h"
#include "msg/sustainnotemsg.h"

/**
 * Message filter that suppresses note-offs for the notes a sustain request covers.
 *
 * Its RTTI descriptor is at `0x008f2a70`. It derives from MsgSink at offset 0. The base vptr lands
 * at `+0x00` and the class is 0x20 bytes. Its vtable is at `0x007e5b00` and runs the type
 * function, the destructor, the inherited MsgSink::Dispatch(), and DispatchPriv().
 *
 * Two sets of note numbers drive the filter. The sounding set records every note a note-on has let
 * through, and the sustained set records every note a SustainNoteMsg has requested the filter
 * sustain. A note-off for a sustained note is discarded, a note-on for a sustained note drops it
 * from the sustained set instead of going downstream, and every other MIDI message goes
 * downstream unchanged.
 *
 * The descriptor at `0x008f2a70` is what settles the name.
 */
class SynthSustainer : public MsgSink {
private:
    /**
     * Add the requested note to the sustained set unless it is already sounding.
     *
     * @ghidraAddress NTSC-U/C: 0x001d20a0
     * @ghidraAddress PAL: 0x001d7f58
     */
    void HandleSustainNote(SustainNoteMsg *pMsg);

    /**
     * Filter a raw MIDI message against the two sets.
     *
     * @ghidraAddress NTSC-U/C: 0x001d2160
     * @ghidraAddress PAL: 0x001d8018
     */
    void HandleStdMidi(StdMidiMsg *pMsg);

    std::vector<unsigned char> mSounding;  // +0x04, notes a note-on has let through
    std::vector<unsigned char> mSustained; // +0x10, notes a sustain request is sustaining

public:
    /**
     * Construct a filter with both sets empty and no downstream sink.
     *
     * @ghidraAddress NTSC-U/C: 0x001d2060
     * @ghidraAddress PAL: 0x001d7f18
     */
    SynthSustainer();

    /**
     * @ghidraAddress NTSC-U/C: 0x001d2860
     * @ghidraAddress PAL: 0x001d8718
     */
    virtual ~SynthSustainer();

    /**
     * Sink that receives every message the filter passes on.
     *
     * The constructor zeroes it and nothing in this translation unit assigns it, so the writer is
     * outside the class and the member is recorded public. A friend declaration on whichever
     * class builds the filter fits the image equally well. The declaration sits after the two
     * vectors because the member sits after them in the object.
     */
    MsgSink *mSink; /*!< The downstream sink. +0x1c */

protected:
    /**
     * Dispatch on the message's registered identity.
     *
     * Vtable slot 3. A SustainNoteMsg and a StdMidiMsg arrive at their handlers and every other
     * message is discarded.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x001d2a10
     * @ghidraAddress PAL: 0x001d88c8
     */
    virtual void DispatchPriv(Message *pMsg);
};
