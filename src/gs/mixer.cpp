#include "gs/mixer.h"

#include "msg/stdmidimsg.h"
#include "msg/trackselectmsg.h"
#include "msg/tracksonmsg.h"

namespace {

constexpr unsigned char kStatusControlChange = 0xb0;
constexpr unsigned char kControllerPan = 10;
constexpr unsigned char kControllerExpression = 11;
constexpr unsigned char kControllerMute = 0x2e;
constexpr unsigned char kControllerFirstGain = 0x2f;
constexpr unsigned char kControllerLastGain = 0x32;
constexpr unsigned char kMaxLevel = 127;

// 127 cubed, which is what normalises the product of the four gain factors back into seven bits.
constexpr int kGainDivisor = 0x1f417f;

// Pan the section index maps to. The index is three bits, so entries 6 and 7 fall outside the
// table and send zero.
constexpr unsigned char kSectionPan[] = {0, 0, 0x20, 0x40, 0x60, kMaxLevel};

} // namespace

// 0x001a8130
Mixer::~Mixer() {
}

// 0x001a72b8
void Mixer::SendPan() {
    const unsigned nIndex = static_cast<unsigned>(mTrack - mLastSection) & 7;
    unsigned char nPan = 0;
    if (nIndex < sizeof(kSectionPan)) {
        nPan = kSectionPan[nIndex];
    }

    StdMidiMsg msg;
    msg.mUnknown08 = kStatusControlChange | mChannel;
    msg.mUnknown09 = kControllerPan;
    msg.mUnknown0a = nPan;
    mOutput->Handle(&msg);
}

// 0x001a73a8
void Mixer::SetGainFactor(int nIndex, unsigned char nFactor) {
    mGainFactors[nIndex] = nFactor;

    const unsigned char nLevel =
        static_cast<unsigned char>(static_cast<unsigned>(mGainFactors[0] * mGainFactors[1] *
                                                         mGainFactors[2] * mGainFactors[3]) /
                                   kGainDivisor);
    if (nLevel == mLevel || mMuted != 0) {
        return;
    }
    mLevel = nLevel;

    StdMidiMsg msg;
    msg.mUnknown08 = kStatusControlChange | mChannel;
    msg.mUnknown09 = kControllerExpression;
    msg.mUnknown0a = mLevel;
    mOutput->Handle(&msg);
}

// 0x001a7490
void Mixer::SetMuted(int bMuted) {
    const int bWasMuted = mMuted;
    mMuted = bMuted;

    if (bWasMuted != 0) {
        if (bMuted == 0) {
            StdMidiMsg msg;
            msg.mUnknown08 = kStatusControlChange | mChannel;
            msg.mUnknown09 = kControllerExpression;
            msg.mUnknown0a = mLevel;
            mOutput->Handle(&msg);
        }
        return;
    }

    if (bMuted == 0) {
        return;
    }

    StdMidiMsg msg;
    msg.mUnknown08 = kStatusControlChange | mChannel;
    msg.mUnknown09 = kControllerExpression;
    msg.mUnknown0a = 0;
    mOutput->Handle(&msg);
}

// 0x001a8390
void Mixer::ApplyControlChange(StdMidiMsg *pMsg) {
    const unsigned char nController = pMsg->mUnknown09;
    if (nController >= kControllerFirstGain && nController <= kControllerLastGain) {
        SetGainFactor(nController - kControllerFirstGain, pMsg->mUnknown0a);
        return;
    }
    if (nController == kControllerMute) {
        SetMuted(pMsg->mUnknown0a != 0);
        return;
    }
    if (nController == kControllerExpression) {
        return;
    }
    if (nController == kControllerPan && mOwnsPan != 0) {
        return;
    }
    mOutput->Handle(pMsg);
}

// 0x001a7780
void Mixer::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_dwStdMidiMsgType)) {
        StdMidiMsg *pMidi = static_cast<StdMidiMsg *>(pMsg);
        if ((pMidi->mUnknown08 & 0xf0) == kStatusControlChange) {
            ApplyControlChange(pMidi);
            return;
        }
        mOutput->Handle(pMsg);
        return;
    }
    if (nType == static_cast<int>(g_dwTracksOnMsgType)) {
        OnTracksOn(pMsg);
        return;
    }
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        OnTrackSelect(pMsg);
    }
}
