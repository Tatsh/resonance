#include "msg/pointamountmsg.h"

#include <iostream>

#include "game/player.h"

Message *PointAmountMsg::New() {
    return new PointAmountMsg;
}

Message *PointAmountMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new PointAmountMsg(*this);
}

int PointAmountMsg::Type() {
    return g_nPointAmountMsgType;
}

const char *PointAmountMsg::GetName() const {
    return "PointAmountMsg";
}

int PointAmountMsg::GetScore() {
    return mPlayer->GetScore();
}

float PointAmountMsg::GetScoreFraction() {
    return static_cast<float>(mPlayer->GetScore()) / static_cast<float>(mMaxScore);
}

void PointAmountMsg::PrintExtra(std::ostream &stream) const {
    mPlayer->Print(stream);
}
