#pragma once

#include <algorithm>
#include <vector>

#include "app/msgsink.h"
#include "mid/mbt.h"
#include "mid/tickobj.h"
#include "msg/musemsg.h"
#include "msg/notemsg.h"

class Message;

/**
 * Sink that notes whether any note sounds past a position.
 *
 * `10NoteFinder` in the RTTI descriptor at `0x008eeef8`, deriving publicly from MsgSink at offset
 * 0. Its type function is at `0x001055c0` and its table at `0x007ccb98` keeps MsgSink::Handle().
 * The object is 0xc bytes. LocalJamEnableMgr::QueryBar() builds one on its stack and searches the
 * MIDI of one or two bars with it. The destructor at `0x00105588` is implicitly declared.
 */
class NoteFinder : public MsgSink {
public:
    /** Start with no note found and the position at kMBTInfinity. */
    NoteFinder() : mFound(0) {
    }

    /**
     * Pass every message of a MIDI range through Handle() against a position.
     *
     * LocalJamEnableMgr::QueryBar() expands the body twice. The title is inferred.
     *
     * @param pMidi The range.
     * @param tick The position a note has to sound past.
     * @ghidraAddress 0x00105660
     */
    void Search(const std::vector<TickObj<MuseMsg *> > *pMidi, Mid::MBT tick) {
        mTick = tick;
        for (std::vector<TickObj<MuseMsg *> >::const_iterator it = pMidi->begin();
             it != pMidi->end();
             ++it) {
            Handle(it->mValue);
        }
    }

    /**
     * Pass a NoteMsg to OnNote() and ignore every other message.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001023b0
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Non-zero once a note sounds past mTick.
     *
     * Public because LocalJamEnableMgr::QueryBar() reads it directly, and the image has no
     * accessor.
     */
    int mFound;

private:
    /**
     * Set mFound when a note sounds past mTick.
     *
     * A note sounds past mTick when mTick lies before the note's start or before its end.
     * HandleMessage() expands the body. The title is inferred.
     *
     * @param pNote The note.
     * @ghidraAddress 0x001056d8
     */
    void OnNote(NoteMsg *pNote) {
        bool bSounding = false;
        if (mTick.mTick < pNote->mTick) {
            bSounding = true;
        } else {
            const Mid::MBT end(
                std::min(std::max(pNote->mTick + pNote->mLength.mTick, kMBTMinimum), kMBTMaximum));
            if (mTick.mTick < end.mTick) {
                bSounding = true;
            }
        }
        if (bSounding) {
            mFound = 1;
        }
    }

    Mid::MBT mTick;
};
