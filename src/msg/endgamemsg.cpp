#include "msg/endgamemsg.h"

// NTSC-U/C: 0x003d7b88, PAL: 0x0040faa0
Message *EndGameMsg::New() {
    return new EndGameMsg;
}

// NTSC-U/C: 0x001939b8, PAL: 0x001995e0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *EndGameMsg::Clone() {
    return new EndGameMsg(*this);
}

// NTSC-U/C: 0x00193a00, PAL: 0x00199628
int EndGameMsg::Type() {
    return g_nEndGameMsgType;
}

// NTSC-U/C: 0x00193a10, PAL: 0x00199638
const char *EndGameMsg::Name() {
    return "EndGameMsg";
}
