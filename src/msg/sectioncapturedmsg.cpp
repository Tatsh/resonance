#include "msg/sectioncapturedmsg.h"

// 0x003dee18. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *SectionCapturedMsg::Clone() {
    return new SectionCapturedMsg(*this);
}

// 0x003dee80
int SectionCapturedMsg::Type() {
    return g_nSectionCapturedMsgType;
}

// 0x003dee90
const char *SectionCapturedMsg::Name() {
    return "SectionCapturedMsg";
}
