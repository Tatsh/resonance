#include "msg/allnotesoffmsg.h"

Message *AllNotesOffMsg::New() {
    return new AllNotesOffMsg;
}

Message *AllNotesOffMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new AllNotesOffMsg(*this);
}

int AllNotesOffMsg::Type() {
    return g_dwAllNotesOffMsgType;
}

const char *AllNotesOffMsg::GetName() const {
    return "AllNotesOffMsg";
}

int AllNotesOffMsg::IsAllNotesOff() {
    return 1;
}
