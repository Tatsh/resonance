#include "msg/stdmidimsg.h"

#include <iostream>

#include "mid/tick.h"
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

// NTSC-U/C: 0x003d6d40, PAL: 0x0040ec30
Message *StdMidiMsg::New() {
    return new StdMidiMsg;
}

// NTSC-U/C: 0x003dbfa0, PAL: 0x004143d8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *StdMidiMsg::Clone() {
    return new StdMidiMsg(*this);
}

// NTSC-U/C: 0x003dc010, PAL: 0x00414448
int StdMidiMsg::Type() {
    return sID;
}

// NTSC-U/C: 0x003dc020, PAL: 0x00414458
const char *StdMidiMsg::GetName() const {
    return "StdMidiMsg";
}

// NTSC-U/C: 0x003d7ec0, PAL: 0x0040fff8
void StdMidiMsg::PrintExtra(std::ostream &stream) const {
    HxStr kind;
    switch (mStatus & kStatusKindMask) {
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

    Sch::Tick position;
    position.mTick = mTick;
    position.Print(stream);
    stream << ' ' << kind << ' ' << static_cast<int>(mData1) << ' ' << static_cast<int>(mData2)
           << " n" << (mStatus & kStatusChannelMask);
}

// NTSC-U/C: 0x003e3658, PAL: 0x0041b9f8
void StdMidiMsg::saveGuts(OBStream &stream) const {
    unsigned char status = mStatus;
    unsigned char data1 = mData1;
    unsigned char data2 = mData2;
    stream.Write(&status, sizeof(status)).Write(&data1, sizeof(data1)).Write(&data2, sizeof(data2));
}

// NTSC-U/C: 0x003e36e8, PAL: 0x0041ba88
void StdMidiMsg::restoreGuts(IBStream &stream) {
    stream.Read(&mStatus, sizeof(mStatus))
        .Read(&mData1, sizeof(mData1))
        .Read(&mData2, sizeof(mData2));
}
