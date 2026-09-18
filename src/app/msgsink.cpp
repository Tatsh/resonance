#include "app/msgsink.h"

// 0x00105120
MsgSink::~MsgSink() {
}

// 0x00105158
void MsgSink::Handle(Message *pMsg) {
    HandleMessage(pMsg);
}
