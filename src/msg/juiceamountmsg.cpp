#include "msg/juiceamountmsg.h"

#include <iostream>

#include "game/player.h"

Message *JuiceAmountMsg::New() {
    return new JuiceAmountMsg;
}

Message *JuiceAmountMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new JuiceAmountMsg(*this);
}

int JuiceAmountMsg::Type() {
    return g_nJuiceAmountMsgType;
}

const char *JuiceAmountMsg::GetName() const {
    return "JuiceAmountMsg";
}

int JuiceAmountMsg::GetJuice() {
    return mPlayer->GetJuice();
}

float JuiceAmountMsg::GetJuiceFraction() {
    return static_cast<float>(mPlayer->GetJuice()) / static_cast<float>(mMaxJuice);
}

void JuiceAmountMsg::PrintExtra(std::ostream &stream) const {
    mPlayer->Print(stream);
}
