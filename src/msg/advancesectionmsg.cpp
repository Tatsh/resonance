#include "msg/advancesectionmsg.h"

// 0x003d6ba8
Message *AdvanceSectionMsg::New() {
    return new AdvanceSectionMsg;
}

// 0x0011d698. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AdvanceSectionMsg::Clone() {
    return new AdvanceSectionMsg(*this);
}

// 0x0011d6f0
int AdvanceSectionMsg::Type() {
    return g_nAdvanceSectionMsgType;
}

// 0x0011d700
const char *AdvanceSectionMsg::Name() {
    return "AdvanceSectionMsg";
}
