#include "msg/phrasemsg.h"

#include <iostream>

Message *PhraseMsg::New() {
    return new PhraseMsg;
}

Message *PhraseMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new PhraseMsg(*this);
}

int PhraseMsg::Type() {
    return sID;
}

const char *PhraseMsg::GetName() const {
    return "PhraseMsg";
}

void PhraseMsg::PrintExtra(std::ostream &stream) const {
    stream << static_cast<void *>(mPhrase) << " [" << mBar << "]";
}
