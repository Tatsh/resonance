#include "msg/axisregistermsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *AxisRegisterMsg::New() {
    return new AxisRegisterMsg;
}

Message *AxisRegisterMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new AxisRegisterMsg(*this);
}

int AxisRegisterMsg::Type() {
    return g_nAxisRegisterMsgType;
}

const char *AxisRegisterMsg::GetName() const {
    return "AxisRegisterMsg";
}

void AxisRegisterMsg::PrintExtra(std::ostream &stream) const {
    mPosition.Print(stream);
    // The colour name is copied into a temporary before it is written.
    stream << " " << HxStr(mPlayer->mColorName) << " " << mValue;
}
