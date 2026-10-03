#include "app/msgsink.h"

// NTSC-U/C: 0x00105120, PAL: 0x00105120
MsgSink::~MsgSink() {
}

// NTSC-U/C: 0x00105158, PAL: 0x00105158
void MsgSink::Dispatch(Message *pMsg) {
    DispatchPriv(pMsg);
}
