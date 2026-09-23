#include "msg/sustainnotemsg.h"

#include <iostream>

#include "mid/mbt.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003d6e50
Message *SustainNoteMsg::New() {
    return new SustainNoteMsg;
}

// 0x003dc778
// The field copies are the compiler expanding the implicit copy
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

// 0x003e3a18
void SustainNoteMsg::Print(std::ostream &stream) {
    Mid::MBT position;
    position.mTick = mTick;
    position.Print(stream);
    stream << ' ';
    stream << static_cast<char>(mUnknown08);
}

// 0x003e3a70
void SustainNoteMsg::Save(OBStream &stream) {
    unsigned char note = mUnknown08;
    stream.WriteBytes(&note, sizeof(note));
}

// 0x003e3ab0
void SustainNoteMsg::Load(IBStream &stream) {
    stream.ReadBytes(&mUnknown08, sizeof(mUnknown08));
}
