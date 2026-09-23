#include "msg/neutralizemsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d7768
Message *NeutralizeMsg::New() {
    return new NeutralizeMsg;
}

// 0x003e03a8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *NeutralizeMsg::Clone() {
    return new NeutralizeMsg(*this);
}

// 0x003e0418
int NeutralizeMsg::Type() {
    return g_nNeutralizeMsgType;
}

// 0x003e0428
const char *NeutralizeMsg::Name() {
    return "NeutralizeMsg";
}

// 0x003e3ea8. The colour name is copied into a temporary before it is written.
void NeutralizeMsg::Print(std::ostream &stream) {
    stream << HxStr(mPlayer->mColorName);
}
