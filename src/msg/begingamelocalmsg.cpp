#include "msg/begingamelocalmsg.h"

// NTSC-U/C: 0x003d7b50, PAL: 0x0040fa68
Message *BeginGameLocalMsg::New() {
    return new BeginGameLocalMsg;
}

// NTSC-U/C: 0x0010ba48, PAL: 0x0010bbd0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *BeginGameLocalMsg::Clone() {
    return new BeginGameLocalMsg(*this);
}

// NTSC-U/C: 0x0010ba80, PAL: 0x0010bc08
int BeginGameLocalMsg::Type() {
    return g_nBeginGameLocalMsgType;
}

// NTSC-U/C: 0x0010ba90, PAL: 0x0010bc18
const char *BeginGameLocalMsg::GetName() const {
    return "BeginGameLocalMsg";
}
