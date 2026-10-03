#include "msg/contctrlmsg.h"

#include <iostream>

// NTSC-U/C: 0x003d7640, PAL: 0x0040f540
Message *ContCtrlMsg::New() {
    return new ContCtrlMsg;
}

// NTSC-U/C: 0x003dfc28, PAL: 0x00418080
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ContCtrlMsg::Clone() {
    return new ContCtrlMsg(*this);
}

// NTSC-U/C: 0x003dfc80, PAL: 0x004180d8
int ContCtrlMsg::Type() {
    return g_nContCtrlMsgType;
}

// NTSC-U/C: 0x003dfc90, PAL: 0x004180e8
const char *ContCtrlMsg::Name() {
    return "ContCtrlMsg";
}

// NTSC-U/C: 0x003e3e80, PAL: 0x0041c050
void ContCtrlMsg::Print(std::ostream &stream) {
    stream << mValue;
}
