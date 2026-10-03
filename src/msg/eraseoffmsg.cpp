#include "msg/eraseoffmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d6b68, PAL: 0x0040ea58
Message *EraseOffMsg::New() {
    return new EraseOffMsg;
}

// NTSC-U/C: 0x003db5b8, PAL: 0x004139f0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *EraseOffMsg::Clone() {
    return new EraseOffMsg(*this);
}

// NTSC-U/C: 0x003db610, PAL: 0x00413a48
int EraseOffMsg::Type() {
    return g_nEraseOffMsgType;
}

// NTSC-U/C: 0x003db620, PAL: 0x00413a58
const char *EraseOffMsg::Name() {
    return "EraseOffMsg";
}

// NTSC-U/C: 0x003e33d0, PAL: 0x0041b710
// The colour name is copied into a temporary before it is written.
void EraseOffMsg::Print(std::ostream &stream) {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName);
}
