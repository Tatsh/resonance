#include "game/riffrangefinder.h"

#include "msg/notemsg.h"

// 0x001c4538
void RiffRangeFinder::HandleMessage(Message *pMsg) {
    if (pMsg->Type() != static_cast<int>(g_dwNoteMsgType)) {
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
