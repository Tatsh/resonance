#include "msg/displaypointermsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// No address of its own. GamePowerupPlacer expands it into six call sites.
DisplayPointerMsg::DisplayPointerMsg(int nBar, int nPlayerValue, Player *pPlayer)
    : mBar(nBar), mPlayerValue(nPlayerValue), mPlayer(pPlayer) {
}

// NTSC-U/C: 0x003d70e0, PAL: 0x0040efd0
Message *DisplayPointerMsg::New() {
    return new DisplayPointerMsg;
}

// NTSC-U/C: 0x003dd980, PAL: 0x00415db8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *DisplayPointerMsg::Clone() {
    return new DisplayPointerMsg(*this);
}

// NTSC-U/C: 0x003dd9d8, PAL: 0x00415e10
int DisplayPointerMsg::Type() {
    return g_nDisplayPointerMsgType;
}

// NTSC-U/C: 0x003dd9e8, PAL: 0x00415e20
const char *DisplayPointerMsg::Name() {
    return "DisplayPointerMsg";
}

// NTSC-U/C: 0x003d8358, PAL: 0x004106f0
// The colour name is copied into a temporary before it is written.
void DisplayPointerMsg::Print(std::ostream &stream) {
    if (mPlayerValue == -1) {
        stream << "remove";
    } else {
        stream << "tr# " << mPlayerValue << ":" << mBar << " " << HxStr(mPlayer->mColorName);
    }
}
