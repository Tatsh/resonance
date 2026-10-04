#include "msg/neutralizemsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *NeutralizeMsg::New() {
    return new NeutralizeMsg;
}

Message *NeutralizeMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new NeutralizeMsg(*this);
}

int NeutralizeMsg::Type() {
    return sID;
}

const char *NeutralizeMsg::GetName() const {
    return "NeutralizeMsg";
}

void NeutralizeMsg::PrintExtra(std::ostream &stream) const {
    // The colour name is copied into a temporary before it is written.
    stream << HxStr(mPlayer->mColorName);
}
