#include "msg/notemsg.h"

// 0x003dc208. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *NoteMsg::Clone() {
    return new NoteMsg(*this);
}

// 0x003dc280
int NoteMsg::Type() {
    return g_dwNoteMsgType;
}

// 0x003dc290
const char *NoteMsg::Name() {
    return "NoteMsg";
}
