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

Message *StdMidiMsg::New() {
    return new StdMidiMsg;
}

Message *StdMidiMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new StdMidiMsg(*this);
}

int StdMidiMsg::Type() {
    return sID;
}

const char *StdMidiMsg::GetName() const {
    return "StdMidiMsg";
}

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

void StdMidiMsg::saveGuts(OBStream &stream) const {
    unsigned char status = mStatus;
    unsigned char data1 = mData1;
    unsigned char data2 = mData2;
    stream.Write(&status, sizeof(status)).Write(&data1, sizeof(data1)).Write(&data2, sizeof(data2));
}

void StdMidiMsg::restoreGuts(IBStream &stream) {
    stream.Read(&mStatus, sizeof(mStatus))
        .Read(&mData1, sizeof(mData1))
        .Read(&mData2, sizeof(mData2));
}
