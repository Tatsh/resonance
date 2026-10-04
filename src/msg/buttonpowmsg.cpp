#include "msg/buttonpowmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *ButtonPowMsg::New() {
    return new ButtonPowMsg;
}

Message *ButtonPowMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new ButtonPowMsg(*this);
}

int ButtonPowMsg::Type() {
    return g_nButtonPowMsgType;
}

const char *ButtonPowMsg::GetName() const {
    return "ButtonPowMsg";
}

void ButtonPowMsg::PrintExtra(std::ostream &stream) const {
    mPosition.Print(stream);
    // The colour name is copied into a temporary before it is written.
    stream << " " << HxStr(mPlayer->mColorName) << " " << mPlayMode;
}
