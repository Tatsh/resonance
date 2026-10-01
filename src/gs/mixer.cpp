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

// Pan the three-bit section index maps to.
constexpr unsigned char kSectionPan[] = {0x40, 0x60, kMaxLevel, 0x60, 0x40, 0x20, 0, 0x20};

// The three configuration codes the constructor reads.
constexpr int kOwnsPanConfigCode = 920;
constexpr int kBoostVolumeConfigCode = 921;
constexpr int kTrackLevelsConfigCode = 927;

// The value the constructor gives mTracksOnBar.
constexpr int kNoValue = -1;

// The gain factor RecomputeGain() drives, and the factor it uses once the song is completed.
constexpr int kStateGainFactor = 3;
constexpr unsigned char kCompletedGain = 115;

} // namespace

// 0x001a7110
Mixer::Mixer(int nTrack, unsigned char nChannel)
    : mChannel(nChannel), mTrack(nTrack), mLastSection(0), mSelection(&g_nullPlayer),
      mZeroedBytes(), mLevelIndex(0), mTracksOnBar(kNoValue) {
    mOwnsPan = QueryConfigFlag(kOwnsPanConfigCode);
    mBoostVolume = static_cast<unsigned char>(QueryConfigValue(kBoostVolumeConfigCode));
    mEndBar = Application::shared()->GetPlayMap()->GetEndBar();
    mMuted = 0;
    // Yes, the binary zeroes it again.
    std::fill(mZeroedBytes, mZeroedBytes + sizeof(mZeroedBytes), 0);
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
    msg.mStatus = kStatusControlChange | mChannel;
    msg.mData1 = kControllerPan;
    msg.mData2 = nPan;
    mOutput->Handle(&msg);
}

// 0x001a73a8
void Mixer::SetGainFactor(int nIndex, unsigned char nFactor) {
    mGainFactors[nIndex] = nFactor;

    const unsigned nLevel = static_cast<unsigned>(mGainFactors[0] * mGainFactors[1] *
                                                  mGainFactors[2] * mGainFactors[3]) /
                            kGainDivisor;
    if (nLevel == mLevel) {
        return;
    }
    mLevel = static_cast<unsigned char>(nLevel); // Yes, a muted mixer still takes the new level.
    if (mMuted != 0) {
        return;
    }

    StdMidiMsg msg;
    msg.mStatus = kStatusControlChange | mChannel;
    msg.mData1 = kControllerExpression;
    msg.mData2 = mLevel;
    mOutput->Handle(&msg);
}

// 0x001a7490
void Mixer::SetMuted(int bMuted) {
    const int bWasMuted = mMuted;
    mMuted = bMuted;

    if (bWasMuted != 0) {
        if (bMuted == 0) {
            StdMidiMsg msg;
            msg.mStatus = kStatusControlChange | mChannel;
            msg.mData1 = kControllerExpression;
            msg.mData2 = mLevel;
            mOutput->Handle(&msg);
        }
        return;
    }

    if (bMuted == 0) {
        return;
    }

    StdMidiMsg msg;
    msg.mStatus = kStatusControlChange | mChannel;
    msg.mData1 = kControllerExpression;
    msg.mData2 = 0;
    mOutput->Handle(&msg);
}

// 0x001a75d8
void Mixer::OnTrackSelect(TrackSelectMsg *pMsg) {
    if (pMsg->mTrack == mTrack) {
        mSelection = pMsg->mPlayer;
        RecomputeGain();
    }
    if (mSelection->GetInputSlot() != 0) {
        return;
    }
    mLastSection = pMsg->mTrack;
    if (mOwnsPan != 0) {
        SendPan();
    }
}

// 0x001a76d0
void Mixer::OnTracksOn(TracksOnMsg *pMsg) {
    mTracksOnBar = pMsg->mBar;
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
    const unsigned char nController = pMsg->mData1;
    if (nController >= kControllerFirstGain && nController <= kControllerLastGain) {
        SetGainFactor(nController - kControllerFirstGain, pMsg->mData2);
        return;
    }
    if (nController == kControllerMute) {
        SetMuted(pMsg->mData2 != 0);
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
        if ((pMidi->mStatus & 0xf0) == kStatusControlChange) {
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
