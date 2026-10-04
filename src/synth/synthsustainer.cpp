#include "synth/synthsustainer.h"

#include <algorithm>

namespace {

constexpr unsigned char kMidiStatusMask = 0xf0;
constexpr unsigned char kMidiNoteOff = 0x80;
constexpr unsigned char kMidiNoteOn = 0x90;

} // namespace

// NTSC-U/C: 0x001d2060, PAL: 0x001d7f18
SynthSustainer::SynthSustainer() : mSink(nullptr) {
}

// NTSC-U/C: 0x001d2860, PAL: 0x001d8718
SynthSustainer::~SynthSustainer() {
}

// NTSC-U/C: 0x001d2a10, PAL: 0x001d88c8
void SynthSustainer::DispatchPriv(Message *pMsg) {
    unsigned dwType = pMsg->Type();
    if (dwType == SustainNoteMsg::sID) {
        HandleSustainNote(static_cast<SustainNoteMsg *>(pMsg));
    } else if (dwType == StdMidiMsg::sID) {
        HandleStdMidi(static_cast<StdMidiMsg *>(pMsg));
    }
}

// NTSC-U/C: 0x001d20a0, PAL: 0x001d7f58
void SynthSustainer::HandleSustainNote(SustainNoteMsg *pMsg) {
    (void)std::find(mSustained.begin(),
                    mSustained.end(),
                    pMsg->mNote); // Yes, the binary discards this result.
    if (std::find(mSounding.begin(), mSounding.end(), pMsg->mNote) != mSounding.end()) {
        mSustained.push_back(pMsg->mNote);
    }
}

// NTSC-U/C: 0x001d2160, PAL: 0x001d8018
void SynthSustainer::HandleStdMidi(StdMidiMsg *pMsg) {
    unsigned char nStatus = pMsg->mStatus;
    unsigned char nNote = pMsg->mData1;

    if ((nStatus & kMidiStatusMask) == kMidiNoteOff) {
        if (std::find(mSustained.begin(), mSustained.end(), nNote) != mSustained.end()) {
            return;
        }
        mSink->Dispatch(pMsg);
        std::vector<unsigned char>::iterator sounding =
            std::find(mSounding.begin(), mSounding.end(), nNote);
        if (sounding != mSounding.end()) {
            mSounding.erase(sounding);
        }
    } else if ((nStatus & kMidiStatusMask) == kMidiNoteOn) {
        std::vector<unsigned char>::iterator held =
            std::find(mSustained.begin(), mSustained.end(), nNote);
        if (held == mSustained.end()) {
            mSink->Dispatch(pMsg);
            mSounding.push_back(nNote);
        } else {
            mSustained.erase(held);
        }
    } else {
        mSink->Dispatch(pMsg);
    }
}
