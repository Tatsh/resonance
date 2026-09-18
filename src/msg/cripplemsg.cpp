#include "msg/cripplemsg.h"

// 0x003e2788. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *CrippleMsg::Clone() {
    return new CrippleMsg(*this);
}

// 0x003e27f8
int CrippleMsg::Type() {
    return g_nCrippleMsgType;
}

// 0x003e2808
const char *CrippleMsg::Name() {
    return "CrippleMsg";
}
