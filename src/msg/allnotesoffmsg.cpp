#include "msg/allnotesoffmsg.h"

// NTSC-U/C: 0x003d6dc8, PAL: 0x0040ecb8
Message *AllNotesOffMsg::New() {
    return new AllNotesOffMsg;
}

// NTSC-U/C: 0x0019a618, PAL: 0x001a0380
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AllNotesOffMsg::Clone() {
    return new AllNotesOffMsg(*this);
}

// NTSC-U/C: 0x0019a670, PAL: 0x001a03d8
int AllNotesOffMsg::Type() {
    return g_dwAllNotesOffMsgType;
}

// NTSC-U/C: 0x0019a680, PAL: 0x001a03e8
const char *AllNotesOffMsg::GetName() const {
    return "AllNotesOffMsg";
}

// NTSC-U/C: 0x0019a690, PAL: 0x001a03f8
int AllNotesOffMsg::IsAllNotesOff() {
    return 1;
}
