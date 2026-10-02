#pragma once

#include "app/msgsink.h"
#include "gs/multimuse.h"
#include "msg/message.h"

/**
 * Visitor that finds the lowest and highest note of a sequence.
 *
 * `15RiffRangeFinder` in the RTTI descriptor at `0x008ef058`, with MsgSink as its only base at
 * offset 0. The object is 0xc bytes. PitchPicker::FindRiffRange() builds one on its stack.
 *
 * The destructor at `0x001c4200` is implicitly declared. It restores MsgSink's table and, for the
 * deleting variant, releases the object under MsgSink's tag.
 */
class RiffRangeFinder : public MsgSink {
public:
    /**
     * Visit every message of a sequence and report the note range.
     *
     * Inline. PitchPicker::FindRiffRange() expands it, and the image also keeps an out-of-line
     * copy. The range starts at 127 for the low end and zero for the high end, which a sequence
     * without a NoteMsg reports unchanged.
     *
     * @param pMuse The sequence.
     * @param pLow Receives the lowest note number.
     * @param pHigh Receives the highest note number.
     * @ghidraAddress 0x001c42b0
     */
    RiffRangeFinder(MultiMuse *pMuse, int *pLow, int *pHigh) : mLow(kHighestNote), mHigh(0) {
        for (const auto &entry : pMuse->mEntries) {
            HandleMessage(entry.mValue);
        }
        *pLow = mLow;
        *pHigh = mHigh;
    }

    /**
     * Widen the range to the note of a NoteMsg.
     *
     * Every other message is ignored.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001c4538
     */
    virtual void HandleMessage(Message *pMsg);

private:
    static constexpr unsigned int kHighestNote = 127;

    unsigned int mLow;  // +0x04
    unsigned int mHigh; // +0x08
};
