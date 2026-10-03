#include "msg/gemmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d74f8, PAL: 0x0040f3f8
Message *GemMsg::New() {
    return new GemMsg;
}

// NTSC-U/C: 0x003df540, PAL: 0x00417998
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *GemMsg::Clone() {
    return new GemMsg(*this);
}

// NTSC-U/C: 0x003df5a8, PAL: 0x00417a00
int GemMsg::Type() {
    return g_nGemMsgType;
}

// NTSC-U/C: 0x003df5b8, PAL: 0x00417a10
const char *GemMsg::GetName() const {
    return "GemMsg";
}

// NTSC-U/C: 0x003d8830, PAL: 0x00410c48
// The colour name is copied into a temporary before it is written.
void GemMsg::PrintExtra(std::ostream &stream) const {
    std::ostream &rest = stream << mTrack << " ";
    mPosition.Print(rest);
    rest << " " << mGem << " " << HxStr(mPlayer->mColorName);
}
