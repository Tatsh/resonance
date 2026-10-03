#include "msg/fadegamemsg.h"

// NTSC-U/C: 0x003d7978, PAL: 0x0040f878
Message *FadeGameMsg::New() {
    return new FadeGameMsg;
}

// NTSC-U/C: 0x00193f88, PAL: 0x00199bc0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *FadeGameMsg::Clone() {
    return new FadeGameMsg(*this);
}

// NTSC-U/C: 0x00193fd8, PAL: 0x00199c10
int FadeGameMsg::Type() {
    return g_nFadeGameMsgType;
}

// NTSC-U/C: 0x00193fe8, PAL: 0x00199c20
const char *FadeGameMsg::GetName() const {
    return "FadeGameMsg";
}
