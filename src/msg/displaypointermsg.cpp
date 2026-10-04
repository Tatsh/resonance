#include "msg/displaypointermsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

DisplayPointerMsg::DisplayPointerMsg(int nBar, int nPlayerValue, Player *pPlayer)
    : mBar(nBar), mPlayerValue(nPlayerValue), mPlayer(pPlayer) {
}

Message *DisplayPointerMsg::New() {
    return new DisplayPointerMsg;
}

Message *DisplayPointerMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new DisplayPointerMsg(*this);
}

int DisplayPointerMsg::Type() {
    return g_nDisplayPointerMsgType;
}

const char *DisplayPointerMsg::GetName() const {
    return "DisplayPointerMsg";
}

void DisplayPointerMsg::PrintExtra(std::ostream &stream) const {
    if (mPlayerValue == -1) {
        stream << "remove";
    } else {
        // The colour name is copied into a temporary before it is written.
        stream << "tr# " << mPlayerValue << ":" << mBar << " " << HxStr(mPlayer->mColorName);
    }
}
