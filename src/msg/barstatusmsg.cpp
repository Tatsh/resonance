#include "msg/barstatusmsg.h"

// 0x003defd0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *BarStatusMsg::Clone() {
    return new BarStatusMsg(*this);
}

// 0x003df050
int BarStatusMsg::Type() {
    return g_nBarStatusMsgType;
}

// 0x003df060
const char *BarStatusMsg::Name() {
    return "BarStatusMsg";
}

// 0x003df1f8
int BarStatusMsg::Has(int nField) {
    return (mFlags & nField) != 0;
}
