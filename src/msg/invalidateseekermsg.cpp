#include "msg/invalidateseekermsg.h"

Message *InvalidateSeekerMsg::New() {
    return new InvalidateSeekerMsg;
}

Message *InvalidateSeekerMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new InvalidateSeekerMsg(*this);
}

int InvalidateSeekerMsg::Type() {
    return sID;
}

const char *InvalidateSeekerMsg::GetName() const {
    return "InvalidateSeekerMsg";
}
