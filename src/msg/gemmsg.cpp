#include "msg/gemmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *GemMsg::New() {
    return new GemMsg;
}

Message *GemMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new GemMsg(*this);
}

int GemMsg::Type() {
    return g_nGemMsgType;
}

const char *GemMsg::GetName() const {
    return "GemMsg";
}

void GemMsg::PrintExtra(std::ostream &stream) const {
    std::ostream &rest = stream << mTrack << " ";
    mPosition.Print(rest);
    // The colour name is copied into a temporary before it is written.
    rest << " " << mGem << " " << HxStr(mPlayer->mColorName);
}
