#include "msg/contctrlmsg.h"

#include <iostream>

Message *ContCtrlMsg::New() {
    return new ContCtrlMsg;
}

Message *ContCtrlMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new ContCtrlMsg(*this);
}

int ContCtrlMsg::Type() {
    return g_nContCtrlMsgType;
}

const char *ContCtrlMsg::GetName() const {
    return "ContCtrlMsg";
}

void ContCtrlMsg::PrintExtra(std::ostream &stream) const {
    stream << mValue;
}
