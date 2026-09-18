#include "msg/neutralizemsg.h"

// 0x003e03a8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *NeutralizeMsg::Clone() {
    return new NeutralizeMsg(*this);
}

// 0x003e0418
int NeutralizeMsg::Type() {
    return g_nNeutralizeMsgType;
}

// 0x003e0428
const char *NeutralizeMsg::Name() {
    return "NeutralizeMsg";
}
