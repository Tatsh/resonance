#include "msg/gemmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d74f8
Message *GemMsg::New() {
    return new GemMsg;
}

// 0x003df540. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *GemMsg::Clone() {
    return new GemMsg(*this);
}

// 0x003df5a8
int GemMsg::Type() {
    return g_nGemMsgType;
}

// 0x003df5b8
const char *GemMsg::Name() {
    return "GemMsg";
}

// 0x003d8830. The colour name is copied into a temporary before it is written.
void GemMsg::Print(std::ostream &stream) {
    std::ostream &rest = stream << mTrack << " ";
    mPosition.Print(rest);
    rest << " " << mGem << " " << HxStr(mPlayer->mColorName);
}
