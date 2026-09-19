#include "msg/pausegamesystemmsg.h"

// 0x003d7bc0
Message *PauseGameSystemMsg::New() {
    return new PauseGameSystemMsg;
}

// 0x00193ce0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PauseGameSystemMsg::Clone() {
    return new PauseGameSystemMsg(*this);
}

// 0x00193d18
int PauseGameSystemMsg::Type() {
    return g_nPauseGameSystemMsgType;
}

// 0x00193d28
const char *PauseGameSystemMsg::Name() {
    return "PauseGameSystemMsg";
}
