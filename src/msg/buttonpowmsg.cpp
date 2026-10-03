#include "msg/buttonpowmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d6ae8, PAL: 0x0040e9d8
Message *ButtonPowMsg::New() {
    return new ButtonPowMsg;
}

// NTSC-U/C: 0x003db220, PAL: 0x00413658
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ButtonPowMsg::Clone() {
    return new ButtonPowMsg(*this);
}

// NTSC-U/C: 0x003db278, PAL: 0x004136b0
int ButtonPowMsg::Type() {
    return g_nButtonPowMsgType;
}

// NTSC-U/C: 0x003db288, PAL: 0x004136c0
const char *ButtonPowMsg::Name() {
    return "ButtonPowMsg";
}

// NTSC-U/C: 0x003d7df0, PAL: 0x0040ff08
// The colour name is copied into a temporary before it is written.
void ButtonPowMsg::Print(std::ostream &stream) {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName) << " " << mPlayMode;
}
