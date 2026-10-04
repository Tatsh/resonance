#include "msg/erasemsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *EraseMsg::New() {
    return new EraseMsg;
}

Message *EraseMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new EraseMsg(*this);
}

int EraseMsg::Type() {
    return sID;
}

const char *EraseMsg::GetName() const {
    return "EraseMsg";
}

void EraseMsg::PrintExtra(std::ostream &stream) const {
    mPosition.Print(stream);
    // The colour name is copied into a temporary before it is written.
    stream << " " << HxStr(mPlayer->mColorName);
}
