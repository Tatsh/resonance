#include "msg/unpausegamesystemmsg.h"

// 0x003d7bf8
Message *UnpauseGameSystemMsg::New() {
    return new UnpauseGameSystemMsg;
}

// 0x00311c68. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *UnpauseGameSystemMsg::Clone() {
    return new UnpauseGameSystemMsg(*this);
}

// 0x00311ca0
int UnpauseGameSystemMsg::Type() {
    return g_nUnpauseGameSystemMsgType;
}

// 0x00311cb0
const char *UnpauseGameSystemMsg::Name() {
    return "UnpauseGameSystemMsg";
}
