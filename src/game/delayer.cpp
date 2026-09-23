#include "game/delayer.h"

#include "msg/message.h"

// 0x0040cee8
Delayer::~Delayer() {
}

// 0x0040d0f0
void Delayer::HandleMessage(Message *pMsg) {
    (void)pMsg->Type(); // Yes, the binary discards this call's result.
    Send(pMsg);
}
