#include "game/msgjoiner.h"

// 0x00195b70
void MsgJoiner::HandleMessage(Message *pMsg) {
    Send(pMsg);
}
