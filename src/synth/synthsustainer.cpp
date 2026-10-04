#include "synth/synthsustainer.h"

#include <algorithm>

namespace {

constexpr unsigned char kMidiStatusMask = 0xf0;
constexpr unsigned char kMidiNoteOff = 0x80;
constexpr unsigned char kMidiNoteOn = 0x90;

} // namespace

SynthSustainer::SynthSustainer() : mSink(nullptr) {
}

SynthSustainer::~SynthSustainer() {
}

void SynthSustainer::DispatchPriv(Message *pMsg) {
    unsigned dwType = pMsg->Type();
    if (dwType == SustainNoteMsg::sID) {
        HandleSustainNote(static_cast<SustainNoteMsg *>(pMsg));
    } else if (dwType == StdMidiMsg::sID) {
        HandleStdMidi(static_cast<StdMidiMsg *>(pMsg));
    }
}

void SynthSustainer::HandleSustainNote(SustainNoteMsg *pMsg) {
    (void)std::find(mSustained.begin(),
                    mSustained.end(),
                    pMsg->mNote); // Yes, the binary discards this result.
    if (std::find(mSounding.begin(), mSounding.end(), pMsg->mNote) != mSounding.end()) {
        mSustained.push_back(pMsg->mNote);
    }
}

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
