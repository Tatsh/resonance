#include "msg/begingamelocalmsg.h"

// 0x003d7b50
Message *BeginGameLocalMsg::New() {
    return new BeginGameLocalMsg;
}

// 0x0010ba48
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *BeginGameLocalMsg::Clone() {
    return new BeginGameLocalMsg(*this);
}

// 0x0010ba80
int BeginGameLocalMsg::Type() {
    return g_nBeginGameLocalMsgType;
}

// 0x0010ba90
const char *BeginGameLocalMsg::Name() {
    return "BeginGameLocalMsg";
}
