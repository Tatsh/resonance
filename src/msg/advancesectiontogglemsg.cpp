#include "msg/advancesectiontogglemsg.h"

// 0x003d71d0
Message *AdvanceSectionToggleMsg::New() {
    return new AdvanceSectionToggleMsg;
}

// 0x00115f90
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AdvanceSectionToggleMsg::Clone() {
    return new AdvanceSectionToggleMsg(*this);
}

// 0x00115fe0
int AdvanceSectionToggleMsg::Type() {
    return g_nAdvanceSectionToggleMsgType;
}

// 0x00115ff0
const char *AdvanceSectionToggleMsg::Name() {
    return "AdvanceSectionToggleMsg";
}
