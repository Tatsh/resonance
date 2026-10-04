#include "msg/axisfxmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *AxisFXMsg::New() {
    return new AxisFXMsg;
}

Message *AxisFXMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new AxisFXMsg(*this);
}

int AxisFXMsg::Type() {
    return g_nAxisFXMsgType;
}

const char *AxisFXMsg::GetName() const {
    return "AxisFXMsg";
}

void AxisFXMsg::PrintExtra(std::ostream &stream) const {
    mPosition.Print(stream);
    // The colour name is copied into a temporary before it is written.
    stream << " " << HxStr(mPlayer->mColorName) << " " << mValue;
}
