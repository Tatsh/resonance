#include "msg/invalidateseekermsg.h"

// 0x003d7280
Message *InvalidateSeekerMsg::New() {
    return new InvalidateSeekerMsg;
}

// 0x001160b0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *InvalidateSeekerMsg::Clone() {
    return new InvalidateSeekerMsg(*this);
}

// 0x00116100
int InvalidateSeekerMsg::Type() {
    return g_nInvalidateSeekerMsgType;
}

// 0x00116110
const char *InvalidateSeekerMsg::Name() {
    return "InvalidateSeekerMsg";
}
