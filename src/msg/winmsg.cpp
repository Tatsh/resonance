#include "msg/winmsg.h"

Message *WinMsg::New() {
    return new WinMsg;
}

Message *WinMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new WinMsg(*this);
}

int WinMsg::Type() {
    return g_nWinMsgType;
}

const char *WinMsg::GetName() const {
    return "WinMsg";
}
