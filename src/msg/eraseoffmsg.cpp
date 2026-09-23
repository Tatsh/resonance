#include "msg/eraseoffmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d6b68
Message *EraseOffMsg::New() {
    return new EraseOffMsg;
}

// 0x003db5b8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *EraseOffMsg::Clone() {
    return new EraseOffMsg(*this);
}

// 0x003db610
int EraseOffMsg::Type() {
    return g_nEraseOffMsgType;
}

// 0x003db620
const char *EraseOffMsg::Name() {
    return "EraseOffMsg";
}

// 0x003e33d0. The colour name is copied into a temporary before it is written.
void EraseOffMsg::Print(std::ostream &stream) {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName);
}
