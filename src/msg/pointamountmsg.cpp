#include "msg/pointamountmsg.h"

#include <iostream>

#include "game/player.h"

// NTSC-U/C: 0x003d7850, PAL: 0x0040f750
Message *PointAmountMsg::New() {
    return new PointAmountMsg;
}

// NTSC-U/C: 0x003e0a48, PAL: 0x00418ea0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PointAmountMsg::Clone() {
    return new PointAmountMsg(*this);
}

// NTSC-U/C: 0x003e0a98, PAL: 0x00418ef0
int PointAmountMsg::Type() {
    return g_nPointAmountMsgType;
}

// NTSC-U/C: 0x003e0aa8, PAL: 0x00418f00
const char *PointAmountMsg::Name() {
    return "PointAmountMsg";
}

// NTSC-U/C: 0x003e40d8, PAL: 0x0041c308
int PointAmountMsg::GetScore() {
    return mPlayer->GetScore();
}

// NTSC-U/C: 0x003e40f8, PAL: 0x0041c328
float PointAmountMsg::GetScoreFraction() {
    return static_cast<float>(mPlayer->GetScore()) / static_cast<float>(mMaxScore);
}

// NTSC-U/C: 0x003e4138, PAL: 0x0041c368
void PointAmountMsg::Print(std::ostream &stream) {
    mPlayer->Print(stream);
}
