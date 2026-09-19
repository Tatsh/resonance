#include "msg/choosepowerupmsg.h"

// No address of its own. The two collections expand it into five call sites.
ChoosePowerupMsg::ChoosePowerupMsg(int nIndex, Player *pOwner, int nType)
    : mIndex(nIndex), mOwner(pOwner), mType(nType) {
}

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
