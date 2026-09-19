#include "msg/winmsg.h"

// 0x003d78c0
Message *WinMsg::New() {
    return new WinMsg;
}

// 0x00116368. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *WinMsg::Clone() {
    return new WinMsg(*this);
}

// 0x001163e0
int WinMsg::Type() {
    return g_nWinMsgType;
}

// 0x001163f0
const char *WinMsg::Name() {
    return "WinMsg";
}
