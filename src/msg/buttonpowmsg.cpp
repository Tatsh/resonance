#include "msg/buttonpowmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d6ae8
Message *ButtonPowMsg::New() {
    return new ButtonPowMsg;
}

// 0x003db220. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ButtonPowMsg::Clone() {
    return new ButtonPowMsg(*this);
}

// 0x003db278
int ButtonPowMsg::Type() {
    return g_nButtonPowMsgType;
}

// 0x003db288
const char *ButtonPowMsg::Name() {
    return "ButtonPowMsg";
}

// 0x003d7df0. The colour name is copied into a temporary before it is written.
void ButtonPowMsg::Print(std::ostream &stream) {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName) << " " << mUnknown08;
}
