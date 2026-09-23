#include "msg/stdmidimsg.h"

#include <iostream>

#include "mid/mbt.h"
#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace {

constexpr unsigned char kStatusKindMask = 0xf0;
constexpr unsigned char kStatusChannelMask = 0x0f;

constexpr int kStatusNoteOff = 0x80;
constexpr int kStatusNoteOn = 0x90;
constexpr int kStatusPolyPressure = 0xa0;
constexpr int kStatusControlChange = 0xb0;
constexpr int kStatusProgramChange = 0xc0;
constexpr int kStatusChannelPressure = 0xd0;
constexpr int kStatusPitchBend = 0xe0;
constexpr int kStatusSystem = 0xf0;

} // namespace

// 0x003d6d40
Message *StdMidiMsg::New() {
    return new StdMidiMsg;
}

// 0x003dbfa0
// The field copies are the compiler expanding the implicit copy
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

// 0x003d7ec0
void StdMidiMsg::Print(std::ostream &stream) {
    HxStr kind;
    switch (mUnknown08 & kStatusKindMask) {
    case kStatusNoteOff:
        kind = "off";
        break;
    case kStatusNoteOn:
        kind = "on";
        break;
    case kStatusControlChange:
        kind = "ctl";
        break;
    case kStatusPitchBend:
        kind = "pb";
        break;
    case kStatusProgramChange:
        kind = "prg";
        break;
    case kStatusPolyPressure:
        kind = "poly";
        break;
    case kStatusChannelPressure:
        kind = "pres";
        break;
    case kStatusSystem:
        kind = "sys";
        break;
    }

    Mid::MBT position;
    position.mTick = mTick;
    position.Print(stream);
    stream << ' ' << kind << ' ' << static_cast<int>(mUnknown09) << ' '
           << static_cast<int>(mUnknown0a) << " n" << (mUnknown08 & kStatusChannelMask);
}

// 0x003e3658
void StdMidiMsg::Save(OBStream &stream) {
    unsigned char status = mUnknown08;
    unsigned char data1 = mUnknown09;
    unsigned char data2 = mUnknown0a;
    stream.WriteBytes(&status, sizeof(status))
        .WriteBytes(&data1, sizeof(data1))
        .WriteBytes(&data2, sizeof(data2));
}

// 0x003e36e8
void StdMidiMsg::Load(IBStream &stream) {
    stream.ReadBytes(&mUnknown08, sizeof(mUnknown08))
        .ReadBytes(&mUnknown09, sizeof(mUnknown09))
        .ReadBytes(&mUnknown0a, sizeof(mUnknown0a));
}
