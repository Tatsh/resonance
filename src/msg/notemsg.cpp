#include "msg/notemsg.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *NoteMsg::New() {
    return new NoteMsg;
}

Message *NoteMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new NoteMsg(*this);
}

int NoteMsg::Type() {
    return sID;
}

const char *NoteMsg::GetName() const {
    return "NoteMsg";
}

void NoteMsg::PrintExtra(std::ostream &stream) const {
    Sch::Tick position;
    position.mTick = mTick;
    position.Print(stream);

    std::ostream &rest = stream << ' ' << static_cast<int>(mNote) << ' '
                                << static_cast<int>(mVelocity) << ' ';
    mLength.Print(rest);
    rest << " n" << static_cast<int>(mChannel);
}

void NoteMsg::saveGuts(OBStream &stream) const {
    unsigned char channel = mChannel;
    unsigned char note = mNote;
    unsigned char velocity = mVelocity;
    unsigned short length = static_cast<unsigned short>(mLength.mTick);
    stream.Write(&channel, sizeof(channel))
        .Write(&note, sizeof(note))
        .Write(&velocity, sizeof(velocity))
        .WriteLE(&length, sizeof(length));
}

void NoteMsg::restoreGuts(IBStream &stream) {
    unsigned short length;
    stream.Read(&mChannel, sizeof(mChannel))
        .Read(&mNote, sizeof(mNote))
        .Read(&mVelocity, sizeof(mVelocity))
        .ReadLE(&length, sizeof(length));
    mLength = Sch::Tick(length);
}
