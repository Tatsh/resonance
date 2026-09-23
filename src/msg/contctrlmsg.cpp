#include "msg/contctrlmsg.h"

#include <iostream>

// 0x003d7640
Message *ContCtrlMsg::New() {
    return new ContCtrlMsg;
}

// 0x003dfc28
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ContCtrlMsg::Clone() {
    return new ContCtrlMsg(*this);
}

// 0x003dfc80
int ContCtrlMsg::Type() {
    return g_nContCtrlMsgType;
}

// 0x003dfc90
const char *ContCtrlMsg::Name() {
    return "ContCtrlMsg";
}

// 0x003e3e80
void ContCtrlMsg::Print(std::ostream &stream) {
    stream << mUnknown0c;
}
