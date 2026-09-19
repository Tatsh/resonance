#include "msg/powerupcountmsg.h"

// No address of its own. PowerupCollection expands it into three call sites.
PowerupCountMsg::PowerupCountMsg(int nIndex, int nCount, Player *pOwner)
    : mIndex(nIndex), mCount(nCount), mOwner(pOwner) {
}

// 0x003dd270. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PowerupCountMsg::Clone() {
    return new PowerupCountMsg(*this);
}

// 0x003dd2c8
int PowerupCountMsg::Type() {
    return g_nPowerupCountMsgType;
}

// 0x003dd2d8
const char *PowerupCountMsg::Name() {
    return "PowerupCountMsg";
}
