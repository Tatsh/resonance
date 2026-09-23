#include "msg/juiceamountmsg.h"

#include <iostream>

#include "game/player.h"

// 0x003d7818
Message *JuiceAmountMsg::New() {
    return new JuiceAmountMsg;
}

// 0x003e08a8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *JuiceAmountMsg::Clone() {
    return new JuiceAmountMsg(*this);
}

// 0x003e08f8
int JuiceAmountMsg::Type() {
    return g_nJuiceAmountMsgType;
}

// 0x003e0908
const char *JuiceAmountMsg::Name() {
    return "JuiceAmountMsg";
}

// 0x003e4178
int JuiceAmountMsg::GetJuice() {
    return mUnknown04->GetJuice();
}

// 0x003e4198
float JuiceAmountMsg::GetJuiceFraction() {
    return static_cast<float>(mUnknown04->GetJuice()) / static_cast<float>(mUnknown08);
}

// 0x003e41d8
void JuiceAmountMsg::Print(std::ostream &stream) {
    mUnknown04->Print(stream);
}
