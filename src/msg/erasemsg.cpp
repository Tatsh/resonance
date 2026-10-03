#include "msg/erasemsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d6b28, PAL: 0x0040ea18
Message *EraseMsg::New() {
    return new EraseMsg;
}

// NTSC-U/C: 0x003db3e0, PAL: 0x00413818
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *EraseMsg::Clone() {
    return new EraseMsg(*this);
}

// NTSC-U/C: 0x003db440, PAL: 0x00413878
int EraseMsg::Type() {
    return g_nEraseMsgType;
}

// NTSC-U/C: 0x003db450, PAL: 0x00413888
const char *EraseMsg::Name() {
    return "EraseMsg";
}

// NTSC-U/C: 0x003e3320, PAL: 0x0041b640
// The colour name is copied into a temporary before it is written.
void EraseMsg::Print(std::ostream &stream) {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName);
}
