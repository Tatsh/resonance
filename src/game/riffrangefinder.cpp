#include "game/riffrangefinder.h"

#include "msg/notemsg.h"

// NTSC-U/C: 0x001c4538, PAL: 0x001ca380
void RiffRangeFinder::DispatchPriv(Message *pMsg) {
    if (pMsg->Type() != static_cast<int>(NoteMsg::sID)) {
        return;
    }
    const unsigned int nNote = static_cast<NoteMsg *>(pMsg)->mNote;
    if (nNote < mLow) {
        mLow = nNote;
    }
    if (mHigh < nNote) {
        mHigh = nNote;
    }
}
