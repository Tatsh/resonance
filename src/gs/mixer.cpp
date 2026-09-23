#include "gs/mixer.h"

#include <algorithm>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gamestats.h"
#include "game/nullplayer.h"
#include "game/playmap.h"
#include "msg/stdmidimsg.h"
#include "msg/trackselectmsg.h"
#include "msg/tracksonmsg.h"
#include "script/configquery.h"

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

// The three configuration codes the constructor reads.
constexpr int kOwnsPanConfigCode = 0x398;
constexpr int kUnknown10ConfigCode = 0x399;
constexpr int kTrackLevelsConfigCode = 0x39f;

// The value the constructor gives mUnknown50.
constexpr int kNoValue = -1;

// The gain factor RecomputeGain() drives, and the factor it uses once the song is completed.
constexpr int kStateGainFactor = 3;
constexpr unsigned char kCompletedGain = 115;

} // namespace

// 0x001a7110
Mixer::Mixer(int nTrack, unsigned char nChannel)
    : mChannel(nChannel), mTrack(nTrack), mLastSection(0), mSelection(&g_nullPlayer), mUnknown28(),
      mLevelIndex(0), mUnknown50(kNoValue) {
    mOwnsPan = QueryConfigFlag(kOwnsPanConfigCode);
    mUnknown10 = static_cast<unsigned char>(QueryConfigValue(kUnknown10ConfigCode));
    mUnknown54 = Application::shared()->GetPlayMap()->Slot9();
    mMuted = 0;
    std::fill(mUnknown28, mUnknown28 + sizeof(mUnknown28), 0); // Yes, the binary zeroes it again.
    mLevel = kMaxLevel;
    std::fill(mGainFactors, mGainFactors + sizeof(mGainFactors), kMaxLevel);
    QueryConfigVector(&mTrackLevels, kTrackLevelsConfigCode);
}

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

// 0x001a75d8
void Mixer::OnTrackSelect(TrackSelectMsg *pMsg) {
    if (pMsg->mUnknown04 == mTrack) {
        mSelection = pMsg->mUnknown10;
        RecomputeGain();
    }
    if (mSelection->Slot2() != 0) {
        return;
    }
    mLastSection = pMsg->mUnknown04;
    if (mOwnsPan != 0) {
        SendPan();
    }
}

// 0x001a76d0
void Mixer::OnTracksOn(TracksOnMsg *pMsg) {
    mUnknown50 = pMsg->mBar;
    mLevelIndex = pMsg->mTracks;
    RecomputeGain();
}

// 0x001a82f0
void Mixer::RecomputeGain() {
    unsigned char nGain;
    if (Application::shared()->GetGameManager()->GetStats()->mCompleted != 0) {
        nGain = kCompletedGain;
    } else if (mSelection->IsNull() == 0) {
        nGain = kMaxLevel;
    } else {
        nGain = static_cast<unsigned char>(kMaxLevel - mTrackLevels[mLevelIndex]);
    }
    SetGainFactor(kStateGainFactor, nGain);
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
        OnTracksOn(static_cast<TracksOnMsg *>(pMsg));
        return;
    }
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        OnTrackSelect(static_cast<TrackSelectMsg *>(pMsg));
    }
}
