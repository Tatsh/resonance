#include "app/msgsink.h"

MsgSink::~MsgSink() {
}

void MsgSink::Dispatch(Message *pMsg) {
    DispatchPriv(pMsg);
}
