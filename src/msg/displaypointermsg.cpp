#include "msg/displaypointermsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// No address of its own. GamePowerupPlacer expands it into six call sites.
DisplayPointerMsg::DisplayPointerMsg(int nBar, int nPlayerValue, Player *pPlayer)
    : mBar(nBar), mPlayerValue(nPlayerValue), mPlayer(pPlayer) {
}

// 0x003d70e0
Message *DisplayPointerMsg::New() {
    return new DisplayPointerMsg;
}

// 0x003dd980
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *DisplayPointerMsg::Clone() {
    return new DisplayPointerMsg(*this);
}

// 0x003dd9d8
int DisplayPointerMsg::Type() {
    return g_nDisplayPointerMsgType;
}

// 0x003dd9e8
const char *DisplayPointerMsg::Name() {
    return "DisplayPointerMsg";
}

// 0x003d8358
// The colour name is copied into a temporary before it is written.
void DisplayPointerMsg::Print(std::ostream &stream) {
    if (mPlayerValue == -1) {
        stream << "remove";
    } else {
        stream << "tr# " << mPlayerValue << ":" << mBar << " " << HxStr(mPlayer->mColorName);
    }
}
