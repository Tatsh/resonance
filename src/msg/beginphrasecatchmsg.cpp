#include "msg/beginphrasecatchmsg.h"

// NTSC-U/C: 0x003d7328, PAL: 0x0040f228
Message *BeginPhraseCatchMsg::New() {
    return new BeginPhraseCatchMsg;
}

// NTSC-U/C: 0x0019d7e8, PAL: 0x001a3550
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *BeginPhraseCatchMsg::Clone() {
    return new BeginPhraseCatchMsg(*this);
}

// NTSC-U/C: 0x0019d840, PAL: 0x001a35a8
int BeginPhraseCatchMsg::Type() {
    return g_nBeginPhraseCatchMsgType;
}

// NTSC-U/C: 0x0019d850, PAL: 0x001a35b8
const char *BeginPhraseCatchMsg::GetName() const {
    return "BeginPhraseCatchMsg";
}
