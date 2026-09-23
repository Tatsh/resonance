#include "msg/notemsg.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003d6d80
Message *NoteMsg::New() {
    return new NoteMsg;
}

// 0x003dc208. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *NoteMsg::Clone() {
    return new NoteMsg(*this);
}

// 0x003dc280
int NoteMsg::Type() {
    return g_dwNoteMsgType;
}

// 0x003dc290
const char *NoteMsg::Name() {
    return "NoteMsg";
}

// 0x003e3760
void NoteMsg::Print(std::ostream &stream) {
    Mid::MBT position;
    position.mTick = mTick;
    position.Print(stream);

    std::ostream &rest = stream << ' ' << static_cast<int>(mUnknown09) << ' '
                                << static_cast<int>(mUnknown0a) << ' ';
    mLength.Print(rest);
    rest << " n" << static_cast<int>(mUnknown08);
}

// 0x003d80c0
void NoteMsg::Save(OBStream &stream) {
    unsigned char byte08 = mUnknown08;
    unsigned char byte09 = mUnknown09;
    unsigned char byte0a = mUnknown0a;
    unsigned short length = static_cast<unsigned short>(mLength.mTick);
    stream.WriteBytes(&byte08, sizeof(byte08))
        .WriteBytes(&byte09, sizeof(byte09))
        .WriteBytes(&byte0a, sizeof(byte0a))
        .Write(&length, sizeof(length));
}

// 0x003e3808
void NoteMsg::Load(IBStream &stream) {
    unsigned short length;
    stream.ReadBytes(&mUnknown08, sizeof(mUnknown08))
        .ReadBytes(&mUnknown09, sizeof(mUnknown09))
        .ReadBytes(&mUnknown0a, sizeof(mUnknown0a))
        .Read(&length, sizeof(length));
    mLength = Mid::MBT(length);
}
