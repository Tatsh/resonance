#include "msg/stdmidimsg.h"

// 0x003dbfa0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *StdMidiMsg::Clone() {
    return new StdMidiMsg(*this);
}

// 0x003dc010
int StdMidiMsg::Type() {
    return g_dwStdMidiMsgType;
}

// 0x003dc020
const char *StdMidiMsg::Name() {
    return "StdMidiMsg";
}
