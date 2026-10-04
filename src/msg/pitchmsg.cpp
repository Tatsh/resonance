#include "msg/pitchmsg.h"

Message *PitchMsg::New() {
    return new PitchMsg;
}

Message *PitchMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new PitchMsg(*this);
}

int PitchMsg::Type() {
    return g_nPitchMsgType;
}

const char *PitchMsg::GetName() const {
    return "PitchMsg";
}
