#include "msg/axisxpowmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *AxisXPowMsg::New() {
    return new AxisXPowMsg;
}

Message *AxisXPowMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new AxisXPowMsg(*this);
}

int AxisXPowMsg::Type() {
    return g_nAxisXPowMsgType;
}

const char *AxisXPowMsg::GetName() const {
    return "AxisXPowMsg";
}

void AxisXPowMsg::PrintExtra(std::ostream &stream) const {
    mPosition.Print(stream);
    // The colour name is copied into a temporary before it is written.
    stream << " " << HxStr(mPlayer->mColorName) << " " << mValue;
}
