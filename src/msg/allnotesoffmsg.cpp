#include "msg/allnotesoffmsg.h"

// 0x003d6dc8
Message *AllNotesOffMsg::New() {
    return new AllNotesOffMsg;
}

// 0x0019a618
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AllNotesOffMsg::Clone() {
    return new AllNotesOffMsg(*this);
}

// 0x0019a670
int AllNotesOffMsg::Type() {
    return g_dwAllNotesOffMsgType;
}

// 0x0019a680
const char *AllNotesOffMsg::Name() {
    return "AllNotesOffMsg";
}

// 0x0019a690
int AllNotesOffMsg::OnUnknownSlot8() {
    return 1;
}
