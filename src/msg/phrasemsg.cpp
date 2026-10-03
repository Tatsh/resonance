#include "msg/phrasemsg.h"

#include <iostream>

// NTSC-U/C: 0x003d79b0, PAL: 0x0040f8b0
Message *PhraseMsg::New() {
    return new PhraseMsg;
}

// NTSC-U/C: 0x003e11e8, PAL: 0x00419640
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PhraseMsg::Clone() {
    return new PhraseMsg(*this);
}

// NTSC-U/C: 0x003e1240, PAL: 0x00419698
int PhraseMsg::Type() {
    return g_nPhraseMsgType;
}

// NTSC-U/C: 0x003e1250, PAL: 0x004196a8
const char *PhraseMsg::Name() {
    return "PhraseMsg";
}

// NTSC-U/C: 0x003e42f8, PAL: 0x0041c528
void PhraseMsg::Print(std::ostream &stream) {
    stream << static_cast<void *>(mPhrase) << " [" << mBar << "]";
}
