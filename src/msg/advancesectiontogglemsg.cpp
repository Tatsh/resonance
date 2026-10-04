#include "msg/advancesectiontogglemsg.h"

Message *AdvanceSectionToggleMsg::New() {
    return new AdvanceSectionToggleMsg;
}

Message *AdvanceSectionToggleMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new AdvanceSectionToggleMsg(*this);
}

int AdvanceSectionToggleMsg::Type() {
    return g_nAdvanceSectionToggleMsgType;
}

const char *AdvanceSectionToggleMsg::GetName() const {
    return "AdvanceSectionToggleMsg";
}
