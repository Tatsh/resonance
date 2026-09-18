#include "msg/phrasemsg.h"

// 0x003e11e8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PhraseMsg::Clone() {
    return new PhraseMsg(*this);
}

// 0x003e1240
int PhraseMsg::Type() {
    return g_nPhraseMsgType;
}

// 0x003e1250
const char *PhraseMsg::Name() {
    return "PhraseMsg";
}
