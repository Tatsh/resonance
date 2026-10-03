#include "msg/jameffectmsg.h"

// NTSC-U/C: 0x003d77e0, PAL: 0x0040f6e0
Message *JamEffectMsg::New() {
    return new JamEffectMsg;
}

// NTSC-U/C: 0x001caa00, PAL: 0x001d08b8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *JamEffectMsg::Clone() {
    return new JamEffectMsg(*this);
}

// NTSC-U/C: 0x001caa60, PAL: 0x001d0918
int JamEffectMsg::Type() {
    return g_nJamEffectMsgType;
}

// NTSC-U/C: 0x001caa70, PAL: 0x001d0928
const char *JamEffectMsg::GetName() const {
    return "JamEffectMsg";
}
