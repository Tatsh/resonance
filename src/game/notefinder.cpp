#include "game/notefinder.h"

#include "msg/message.h"

// NTSC-U/C: 0x001023b0, PAL: 0x001023b0
void NoteFinder::DispatchPriv(Message *pMsg) {
    if (pMsg->Type() == static_cast<int>(NoteMsg::sID)) {
        OnNote(static_cast<NoteMsg *>(pMsg));
    }
}
