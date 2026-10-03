#include "msg/streakovermsg.h"

// NTSC-U/C: 0x003d6cd0, PAL: 0x0040ebc0
Message *StreakOverMsg::New() {
    return new StreakOverMsg;
}

// NTSC-U/C: 0x003dbcf8, PAL: 0x00414130
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *StreakOverMsg::Clone() {
    return new StreakOverMsg(*this);
}

// NTSC-U/C: 0x003dbd40, PAL: 0x00414178
int StreakOverMsg::Type() {
    return g_nStreakOverMsgType;
}

// NTSC-U/C: 0x003dbd50, PAL: 0x00414188
const char *StreakOverMsg::GetName() const {
    return "StreakOverMsg";
}
