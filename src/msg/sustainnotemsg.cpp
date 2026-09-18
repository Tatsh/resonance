#include "msg/sustainnotemsg.h"

// 0x003dc778. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *SustainNoteMsg::Clone() {
    return new SustainNoteMsg(*this);
}

// 0x003dc7d8
int SustainNoteMsg::Type() {
    return g_dwSustainNoteMsgType;
}

// 0x003dc7e8
const char *SustainNoteMsg::Name() {
    return "SustainNoteMsg";
}
