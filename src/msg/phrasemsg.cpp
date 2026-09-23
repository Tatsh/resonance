#include "msg/phrasemsg.h"

#include <iostream>

// 0x003d79b0
Message *PhraseMsg::New() {
    return new PhraseMsg;
}

// 0x003e11e8
// The field copies are the compiler expanding the implicit copy
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

// 0x003e42f8
void PhraseMsg::Print(std::ostream &stream) {
    stream << static_cast<void *>(mPhrase) << " [" << mBar << "]";
}
