#include "msg/axisypowmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *AxisYPowMsg::New() {
    return new AxisYPowMsg;
}

Message *AxisYPowMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new AxisYPowMsg(*this);
}

int AxisYPowMsg::Type() {
    return g_nAxisYPowMsgType;
}

const char *AxisYPowMsg::GetName() const {
    return "AxisYPowMsg";
}

void AxisYPowMsg::PrintExtra(std::ostream &stream) const {
    mPosition.Print(stream);
    // The colour name is copied into a temporary before it is written.
    stream << " " << HxStr(mPlayer->mColorName) << " " << mValue;
}
