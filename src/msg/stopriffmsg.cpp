#include "msg/stopriffmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *StopRiffMsg::New() {
    return new StopRiffMsg;
}

Message *StopRiffMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new StopRiffMsg(*this);
}

int StopRiffMsg::Type() {
    return sID;
}

const char *StopRiffMsg::GetName() const {
    return "StopRiffMsg";
}

void StopRiffMsg::PrintExtra(std::ostream &stream) const {
    mPosition.Print(stream);
    // The colour name is copied into a temporary before it is written.
    stream << " " << HxStr(mPlayer->mColorName) << " b#" << mButton;
}
