#include "msg/choosepowerupmsg.h"

// 0x003dd0b8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ChoosePowerupMsg::Clone() {
    return new ChoosePowerupMsg(*this);
}

// 0x003dd110
int ChoosePowerupMsg::Type() {
    return g_nChoosePowerupMsgType;
}

// 0x003dd120
const char *ChoosePowerupMsg::Name() {
    return "ChoosePowerupMsg";
}
