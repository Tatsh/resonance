#include "msg/jameffectmsg.h"

Message *JamEffectMsg::New() {
    return new JamEffectMsg;
}

Message *JamEffectMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new JamEffectMsg(*this);
}

int JamEffectMsg::Type() {
    return sID;
}

const char *JamEffectMsg::GetName() const {
    return "JamEffectMsg";
}
