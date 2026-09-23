#include "msg/jameffectmsg.h"

// 0x003d77e0
Message *JamEffectMsg::New() {
    return new JamEffectMsg;
}

// 0x001caa00
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *JamEffectMsg::Clone() {
    return new JamEffectMsg(*this);
}

// 0x001caa60
int JamEffectMsg::Type() {
    return g_nJamEffectMsgType;
}

// 0x001caa70
const char *JamEffectMsg::Name() {
    return "JamEffectMsg";
}
