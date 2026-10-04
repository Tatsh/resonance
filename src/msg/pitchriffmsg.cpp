#include "msg/pitchriffmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *PitchRiffMsg::New() {
    return new PitchRiffMsg;
}

Message *PitchRiffMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new PitchRiffMsg(*this);
}

int PitchRiffMsg::Type() {
    return sID;
}

const char *PitchRiffMsg::GetName() const {
    return "PitchRiffMsg";
}

void PitchRiffMsg::PrintExtra(std::ostream &stream) const {
    mPosition.Print(stream);
    // The colour name is copied into a temporary before it is written.
    stream << " " << HxStr(mPlayer->mColorName) << " b#" << mButton;
}
