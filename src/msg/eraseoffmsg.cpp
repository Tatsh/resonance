#include "msg/eraseoffmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *EraseOffMsg::New() {
    return new EraseOffMsg;
}

Message *EraseOffMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new EraseOffMsg(*this);
}

int EraseOffMsg::Type() {
    return g_nEraseOffMsgType;
}

const char *EraseOffMsg::GetName() const {
    return "EraseOffMsg";
}

void EraseOffMsg::PrintExtra(std::ostream &stream) const {
    mPosition.Print(stream);
    // The colour name is copied into a temporary before it is written.
    stream << " " << HxStr(mPlayer->mColorName);
}
