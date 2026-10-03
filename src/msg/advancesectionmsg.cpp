#include "msg/advancesectionmsg.h"

// NTSC-U/C: 0x003d6ba8, PAL: 0x0040ea98
Message *AdvanceSectionMsg::New() {
    return new AdvanceSectionMsg;
}

// NTSC-U/C: 0x0011d698, PAL: 0x0011dc20
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AdvanceSectionMsg::Clone() {
    return new AdvanceSectionMsg(*this);
}

// NTSC-U/C: 0x0011d6f0, PAL: 0x0011dc78
int AdvanceSectionMsg::Type() {
    return g_nAdvanceSectionMsgType;
}

// NTSC-U/C: 0x0011d700, PAL: 0x0011dc88
const char *AdvanceSectionMsg::GetName() const {
    return "AdvanceSectionMsg";
}
