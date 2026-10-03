#include "msg/pausegamesystemmsg.h"

// NTSC-U/C: 0x003d7bc0, PAL: 0x0040fad8
Message *PauseGameSystemMsg::New() {
    return new PauseGameSystemMsg;
}

// NTSC-U/C: 0x00193ce0, PAL: 0x00199908
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PauseGameSystemMsg::Clone() {
    return new PauseGameSystemMsg(*this);
}

// NTSC-U/C: 0x00193d18, PAL: 0x00199940
int PauseGameSystemMsg::Type() {
    return g_nPauseGameSystemMsgType;
}

// NTSC-U/C: 0x00193d28, PAL: 0x00199950
const char *PauseGameSystemMsg::GetName() const {
    return "PauseGameSystemMsg";
}
