#include "game/midichase.h"

#include <string.h>

#include "mid/tick.h"
#include "msg/message.h"
#include "msg/musemsg.h"
#include "msg/stdmidimsg.h"

namespace {

// The high nibble of a status byte selects the kind of message, and the low nibble the channel.
constexpr unsigned char kStatusKindMask = 0xf0;
constexpr unsigned char kStatusChannelMask = 0x0f;
constexpr unsigned char kStatusControlChange = 0xb0;
constexpr unsigned char kStatusProgramChange = 0xc0;
constexpr unsigned char kStatusPitchBend = 0xe0;

} // namespace

// NTSC-U/C: 0x001a6888, PAL: 0x001ac5f0
MidiChase::MidiChase() : mChannel(kUnset), mProgram(kUnset), mBendLow(kUnset), mBendHigh(kUnset) {
    memset(mControllers, kUnset, sizeof(mControllers));
}

// NTSC-U/C: 0x001a67d8, PAL: 0x001ac540
MidiChase::~MidiChase() {
}

// NTSC-U/C: 0x001a6660, PAL: 0x001ac3c8
void MidiChase::DispatchPriv(Message *pMsg) {
    if (static_cast<unsigned int>(pMsg->Type()) != StdMidiMsg::sID) {
        return;
    }

    StdMidiMsg *pMidi = static_cast<StdMidiMsg *>(pMsg);
    mChannel = pMidi->mStatus & kStatusChannelMask;
    switch (pMidi->mStatus & kStatusKindMask) {
    case kStatusProgramChange:
        mProgram = pMidi->mData1;
        break;
    case kStatusControlChange:
        mControllers[pMidi->mData1] = pMidi->mData2;
        break;
    case kStatusPitchBend:
        mBendLow = pMidi->mData1;
        mBendHigh = pMidi->mData2;
        break;
    default:
        break;
    }
}

// NTSC-U/C: 0x001a6900, PAL: 0x001ac668
void MidiChase::HandleRange(const TickObj<MuseMsg *> *pBegin, const TickObj<MuseMsg *> *pEnd) {
    for (const TickObj<MuseMsg *> *pItem = pBegin; pItem != pEnd; ++pItem) {
        Dispatch(pItem->mValue);
    }
}

// NTSC-U/C: 0x001a6488, PAL: 0x001ac1f0
void MidiChase::Replay(MsgSink *pSink) {
    for (int i = 0; i < kControllerCount; ++i) {
        if (mControllers[i] != kUnset) {
            StdMidiMsg message(kTickInfinity,
                               mChannel | kStatusControlChange,
                               static_cast<unsigned char>(i),
                               mControllers[i]);
            pSink->Dispatch(&message);
        }
    }

    if (mProgram != kUnset) {
        StdMidiMsg message(kTickInfinity, mChannel | kStatusProgramChange, mProgram, 0);
        pSink->Dispatch(&message);
    }

    if (mBendLow != kUnset) {
        StdMidiMsg message(kTickInfinity, mChannel | kStatusPitchBend, mBendLow, mBendHigh);
        pSink->Dispatch(&message);
    }
}
