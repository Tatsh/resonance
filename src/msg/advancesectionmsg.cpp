#include "msg/advancesectionmsg.h"

Message *AdvanceSectionMsg::New() {
    return new AdvanceSectionMsg;
}

Message *AdvanceSectionMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new AdvanceSectionMsg(*this);
}

int AdvanceSectionMsg::Type() {
    return sID;
}

const char *AdvanceSectionMsg::GetName() const {
    return "AdvanceSectionMsg";
}
