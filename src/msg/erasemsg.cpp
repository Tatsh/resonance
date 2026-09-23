#include "msg/erasemsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d6b28
Message *EraseMsg::New() {
    return new EraseMsg;
}

// 0x003db3e0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *EraseMsg::Clone() {
    return new EraseMsg(*this);
}

// 0x003db440
int EraseMsg::Type() {
    return g_nEraseMsgType;
}

// 0x003db450
const char *EraseMsg::Name() {
    return "EraseMsg";
}

// 0x003e3320
// The colour name is copied into a temporary before it is written.
void EraseMsg::Print(std::ostream &stream) {
    mUnknown08.Print(stream);
    stream << " " << HxStr(mUnknown04->mColorName);
}
