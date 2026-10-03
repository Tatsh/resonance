#include "msg/juiceamountmsg.h"

#include <iostream>

#include "game/player.h"

// NTSC-U/C: 0x003d7818, PAL: 0x0040f718
Message *JuiceAmountMsg::New() {
    return new JuiceAmountMsg;
}

// NTSC-U/C: 0x003e08a8, PAL: 0x00418d00
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *JuiceAmountMsg::Clone() {
    return new JuiceAmountMsg(*this);
}

// NTSC-U/C: 0x003e08f8, PAL: 0x00418d50
int JuiceAmountMsg::Type() {
    return g_nJuiceAmountMsgType;
}

// NTSC-U/C: 0x003e0908, PAL: 0x00418d60
const char *JuiceAmountMsg::Name() {
    return "JuiceAmountMsg";
}

// NTSC-U/C: 0x003e4178, PAL: 0x0041c3a8
int JuiceAmountMsg::GetJuice() {
    return mPlayer->GetJuice();
}

// NTSC-U/C: 0x003e4198, PAL: 0x0041c3c8
float JuiceAmountMsg::GetJuiceFraction() {
    return static_cast<float>(mPlayer->GetJuice()) / static_cast<float>(mMaxJuice);
}

// NTSC-U/C: 0x003e41d8, PAL: 0x0041c408
void JuiceAmountMsg::Print(std::ostream &stream) {
    mPlayer->Print(stream);
}
