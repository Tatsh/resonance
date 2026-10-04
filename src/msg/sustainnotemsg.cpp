#include "msg/sustainnotemsg.h"

#include <iostream>

#include "mid/tick.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *SustainNoteMsg::New() {
    return new SustainNoteMsg;
}

Message *SustainNoteMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new SustainNoteMsg(*this);
}

int SustainNoteMsg::Type() {
    return sID;
}

const char *SustainNoteMsg::GetName() const {
    return "SustainNoteMsg";
}

void SustainNoteMsg::PrintExtra(std::ostream &stream) const {
    Sch::Tick position;
    position.mTick = mTick;
    position.Print(stream);
    stream << ' ';
    stream << static_cast<char>(mNote);
}

void SustainNoteMsg::saveGuts(OBStream &stream) const {
    unsigned char note = mNote;
    stream.Write(&note, sizeof(note));
}

void SustainNoteMsg::restoreGuts(IBStream &stream) {
    stream.Read(&mNote, sizeof(mNote));
}
