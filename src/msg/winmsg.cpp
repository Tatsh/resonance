#include "msg/winmsg.h"

// NTSC-U/C: 0x003d78c0, PAL: 0x0040f7c0
Message *WinMsg::New() {
    return new WinMsg;
}

// NTSC-U/C: 0x00116368, PAL: 0x00116810
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *WinMsg::Clone() {
    return new WinMsg(*this);
}

// NTSC-U/C: 0x001163e0, PAL: 0x00116888
int WinMsg::Type() {
    return g_nWinMsgType;
}

// NTSC-U/C: 0x001163f0, PAL: 0x00116898
const char *WinMsg::Name() {
    return "WinMsg";
}
