#include "game/delayer.h"

#include "msg/message.h"

// NTSC-U/C: 0x0040cee8, PAL: 0x00446928
Delayer::~Delayer() {
}

// NTSC-U/C: 0x0040d0f0, PAL: 0x00446b30
void Delayer::HandleMessage(Message *pMsg) {
    (void)pMsg->Type(); // Yes, the binary discards this call's result.
    Send(pMsg);
}
