#include "game/msgjoiner.h"

void MsgJoiner::DispatchPriv(Message *pMsg) {
    Send(pMsg);
}
