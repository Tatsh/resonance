#include "game/msgjoiner.h"

// NTSC-U/C: 0x00195b70, PAL: 0x0019b808
void MsgJoiner::HandleMessage(Message *pMsg) {
    Send(pMsg);
}
