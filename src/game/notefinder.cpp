#include "game/notefinder.h"

#include "msg/message.h"

// 0x001023b0
void NoteFinder::HandleMessage(Message *pMsg) {
    if (pMsg->Type() == static_cast<int>(g_dwNoteMsgType)) {
        OnNote(static_cast<NoteMsg *>(pMsg));
    }
}
