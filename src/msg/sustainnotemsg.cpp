#include "msg/sustainnotemsg.h"

#include <iostream>

#include "mid/mbt.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003d6e50, PAL: 0x0040ed40
Message *SustainNoteMsg::New() {
    return new SustainNoteMsg;
}

// NTSC-U/C: 0x003dc778, PAL: 0x00414bb0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *SustainNoteMsg::Clone() {
    return new SustainNoteMsg(*this);
}

// NTSC-U/C: 0x003dc7d8, PAL: 0x00414c10
int SustainNoteMsg::Type() {
    return g_dwSustainNoteMsgType;
}

// NTSC-U/C: 0x003dc7e8, PAL: 0x00414c20
const char *SustainNoteMsg::GetName() const {
    return "SustainNoteMsg";
}

// NTSC-U/C: 0x003e3a18, PAL: 0x0041bdb8
void SustainNoteMsg::PrintExtra(std::ostream &stream) const {
    Mid::MBT position;
    position.mTick = mTick;
    position.Print(stream);
    stream << ' ';
    stream << static_cast<char>(mNote);
}

// NTSC-U/C: 0x003e3a70, PAL: 0x0041be10
void SustainNoteMsg::saveGuts(OBStream &stream) const {
    unsigned char note = mNote;
    stream.WriteBytes(&note, sizeof(note));
}

// NTSC-U/C: 0x003e3ab0, PAL: 0x0041be50
void SustainNoteMsg::restoreGuts(IBStream &stream) {
    stream.ReadBytes(&mNote, sizeof(mNote));
}
