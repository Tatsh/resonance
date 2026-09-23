#include "game/midichase.h"

#include <string.h>

#include "mid/mbt.h"
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

// 0x001a6888
MidiChase::MidiChase() : mChannel(kUnset), mProgram(kUnset), mBendLow(kUnset), mBendHigh(kUnset) {
    memset(mControllers, kUnset, sizeof(mControllers));
}

// 0x001a67d8
MidiChase::~MidiChase() {
}

// 0x001a6660
void MidiChase::HandleMessage(Message *pMsg) {
    if (static_cast<unsigned int>(pMsg->Type()) != g_dwStdMidiMsgType) {
        return;
    }

    StdMidiMsg *pMidi = static_cast<StdMidiMsg *>(pMsg);
    mChannel = pMidi->mUnknown08 & kStatusChannelMask;
    switch (pMidi->mUnknown08 & kStatusKindMask) {
    case kStatusProgramChange:
        mProgram = pMidi->mUnknown09;
        break;
    case kStatusControlChange:
        mControllers[pMidi->mUnknown09] = pMidi->mUnknown0a;
        break;
    case kStatusPitchBend:
        mBendLow = pMidi->mUnknown09;
        mBendHigh = pMidi->mUnknown0a;
        break;
    default:
        break;
    }
}

// 0x001a6900
void MidiChase::HandleRange(const TickObj<MuseMsg *> *pBegin, const TickObj<MuseMsg *> *pEnd) {
    for (const TickObj<MuseMsg *> *pItem = pBegin; pItem != pEnd; ++pItem) {
        Handle(pItem->mValue);
    }
}

// 0x001a6488
void MidiChase::Replay(MsgSink *pSink) {
    for (int i = 0; i < kControllerCount; ++i) {
        if (mControllers[i] != kUnset) {
            StdMidiMsg message(kMBTInfinity,
                               mChannel | kStatusControlChange,
                               static_cast<unsigned char>(i),
                               mControllers[i]);
            pSink->Handle(&message);
        }
    }

    if (mProgram != kUnset) {
        StdMidiMsg message(kMBTInfinity, mChannel | kStatusProgramChange, mProgram, 0);
        pSink->Handle(&message);
    }

    if (mBendLow != kUnset) {
        StdMidiMsg message(kMBTInfinity, mChannel | kStatusPitchBend, mBendLow, mBendHigh);
        pSink->Handle(&message);
    }
}
