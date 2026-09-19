#include "synth/synthsustainer.h"

#include <algorithm>

namespace {

constexpr unsigned char kMidiStatusMask = 0xf0;
constexpr unsigned char kMidiNoteOff = 0x80;
constexpr unsigned char kMidiNoteOn = 0x90;

} // namespace

// 0x001d2060
SynthSustainer::SynthSustainer() {
    mSink = nullptr;
}

// 0x001d2860
SynthSustainer::~SynthSustainer() {
}

// 0x001d2a10
void SynthSustainer::HandleMessage(Message *pMsg) {
    unsigned dwType = pMsg->Type();
    if (dwType == g_dwSustainNoteMsgType) {
        HandleSustainNote(static_cast<SustainNoteMsg *>(pMsg));
    } else if (dwType == g_dwStdMidiMsgType) {
        HandleStdMidi(static_cast<StdMidiMsg *>(pMsg));
    }
}

// 0x001d20a0
void SynthSustainer::HandleSustainNote(SustainNoteMsg *pMsg) {
    (void)std::find(mSustained.begin(),
                    mSustained.end(),
                    pMsg->mUnknown08); // Yes, the binary discards this result.
    if (std::find(mSounding.begin(), mSounding.end(), pMsg->mUnknown08) == mSounding.end()) {
        mSustained.push_back(pMsg->mUnknown08);
    }
}

// 0x001d2160
void SynthSustainer::HandleStdMidi(StdMidiMsg *pMsg) {
    unsigned char nStatus = pMsg->mUnknown08;
    unsigned char nNote = pMsg->mUnknown09;

    if ((nStatus & kMidiStatusMask) == kMidiNoteOff) {
        if (std::find(mSustained.begin(), mSustained.end(), nNote) != mSustained.end()) {
            return;
        }
        mSink->Handle(pMsg);
        std::vector<unsigned char>::iterator sounding =
            std::find(mSounding.begin(), mSounding.end(), nNote);
        if (sounding != mSounding.end()) {
            mSounding.erase(sounding);
        }
    } else if ((nStatus & kMidiStatusMask) == kMidiNoteOn) {
        std::vector<unsigned char>::iterator held =
            std::find(mSustained.begin(), mSustained.end(), nNote);
        if (held == mSustained.end()) {
            mSink->Handle(pMsg);
            mSounding.push_back(nNote);
        } else {
            mSustained.erase(held);
        }
    } else {
        mSink->Handle(pMsg);
    }
}
