#include "gs/scratcher.h"

#include <algorithm>

#include "app/application.h"
#include "app/attachment.h"
#include "app/playsound.h"
#include "game/axeoldgemmaker.h"
#include "game/idablebase.h"
#include "game/nullplayer.h"
#include "game/player.h"
#include "game/riff.h"
#include "game/trackdata.h"
#include "gs/museutil.h"
#include "gs/phrasemgr.h"
#include "msg/allnotesoffmsg.h"
#include "msg/axebuttonmsg.h"
#include "msg/axisregistermsg.h"
#include "msg/beginphrasecatchmsg.h"
#include "msg/durgemmsg.h"
#include "msg/erasemsg.h"
#include "msg/gemmsg.h"
#include "msg/invalidateseekermsg.h"
#include "msg/multimusemsg.h"
#include "msg/nowbarmsg.h"
#include "msg/phrasecapturedmsg.h"
#include "msg/pitchmsg.h"
#include "msg/pitchriffmsg.h"
#include "msg/seekermsg.h"
#include "msg/showeraseeffectmsg.h"
#include "msg/trackselectmsg.h"
#include "script/configquery.h"
#include "synth/ps2hardsynth.h"

namespace {

// The value the constructor gives mAnnouncedBar.
constexpr int kNoValue = -1;

// mReadings starts with this many elements, each built from the int zero.
constexpr int kReadingCount = 3;
constexpr int kInitialReading = 0;

// The two configuration codes that decide mSwitchesBanks.
constexpr int kBankSwitchConfigCode = 0x3a4;
constexpr int kBankSwitchOverrideConfigCode = 0x3a1;

// The word at ShowEraseEffectMsg `+0x14` that EraseGemRange() always sets.
constexpr int kEraseEffectFlag = 1;

// The lane a new player's now bar starts on, the middle of the tunnel, and the blend a fresh
// scratch gem starts at.
constexpr float kCenterLane = 0.5f;

constexpr char kInactiveSound[] = "SND_INACTIVE";

constexpr int kBarTicks = 1920;
constexpr int kHalfBeatTicks = 480;

// A scratch gem lasts this many ticks.
constexpr int kScratchGemTicks = 60;

// The word DurGemMsg's +0x18 carries for a scratch gem, and the gem every scratch reports.
constexpr int kScratchDurGemLive = 1;
constexpr int kScratchGem = 1;

// AxeButtonMsg's state for a press, which a scratch sends in both of its first two words.
constexpr int kButtonPressed = 1;

// PostNowBarMsg() maps an axis value from 0..1 onto a position from 1 down to -1, and records the
// direction of the last scratch it detected.
constexpr double kAxisMidpoint = 0.5;
constexpr double kAxisToPosition = -2.0;
constexpr int kScratchForward = 1;
constexpr int kScratchBackward = -1;
constexpr int kScratchNone = 0;

// A position beyond this is a scratch, and one within the dead zone rearms the detector.
constexpr double kScratchThreshold = 0.2;
constexpr double kDeadZone = 0.15;

// The change in position since the oldest reading scales onto a step of up to three.
constexpr double kSpeedScale = 4.5;
constexpr int kMaxStep = 3;

// The backward step is the scaled speed negated and less this, which is what the binary computes.
constexpr int kBackwardStepBias = 4;

// Saturates a tick to the finite range, as the inline Mid::MBT arithmetic does.
inline int ClampTick(int nTick) {
    return std::min(std::max(nTick, kMBTMinimum), kMBTMaximum);
}

constexpr char kEraseStepSound[] = "SND_ERASE_SECTION";
constexpr char kEraseBarSound[] = "SND_ERASE";

} // namespace

// NTSC-U/C: 0x001cf988, PAL: 0x001d5840
Scratcher::Scratcher(PhraseMgr *pPhraseMgr,
                     Quantizer *pQuantizer,
                     Sch::TickClock *pClock,
                     const TrackData *pTrackData)
    : Pitcher(pClock), mPhraseMgr(pPhraseMgr), mQuantizer(pQuantizer), mTrackData(pTrackData),
      mTrack(pTrackData->mIndex), mBarDivisor(pPhraseMgr->mBarTicks), mClock(pClock),
      mId(kIDableUnregistered), mPlayer(&g_nullPlayer), mLastScratchPlayer(&g_nullPlayer),
      mLastGem(0), mAnnouncedBar(kNoValue), mScratchDirection(0),
      mReadings(kReadingCount, kInitialReading), mLastGemEnd(0), mLastGemEndBlend(0.0f),
      mLastStep(0) {
    mSwitchesBanks = 0;
    if (QueryConfigFlag(kBankSwitchConfigCode) != 0) {
        mSwitchesBanks = QueryConfigFlag(kBankSwitchOverrideConfigCode) == 0;
    }
    mChannel = pTrackData->mChannel;
}

// NTSC-U/C: 0x001cfd20, PAL: 0x001d5bd8
void Scratcher::PostNowBarMsg(AxisRegisterMsg *pMsg) {
    if (pMsg->mTrack != mTrack || mPlayer != pMsg->mPlayer) {
        return;
    }
    NowBarMsg nowBar;
    nowBar.mTrack = pMsg->mTrack;
    nowBar.mPlayer = mPlayer;
    nowBar.mLane = 1.0f - pMsg->mValue;
    Send(&nowBar);

    const float flPosition = static_cast<float>((pMsg->mValue - kAxisMidpoint) * kAxisToPosition);
    const unsigned int nReadings = mReadings.size();

    if (flPosition > kScratchThreshold && mScratchDirection != kScratchForward) {
        mScratchDirection = kScratchForward;
        const float flOldest = mReadings[(nReadings + mNewestReading) % nReadings];
        const int nSpeed = static_cast<int>((flPosition - flOldest) * kSpeedScale);
        OnPitchRiff(mLastGem, std::max(std::min(nSpeed, kMaxStep), 1), pMsg->mPosition.mTick);
    }
    if (flPosition < -kScratchThreshold && mScratchDirection != kScratchBackward) {
        mScratchDirection = kScratchBackward;
        const float flOldest = mReadings[(nReadings + mNewestReading) % nReadings];
        const int nSpeed = static_cast<int>((flPosition - flOldest) * kSpeedScale);
        // Yes, the binary negates the speed and then subtracts four.
        const int nStep = -nSpeed - kBackwardStepBias;
        OnPitchRiff(mLastGem, std::max(std::min(nStep, -1), -kMaxStep), pMsg->mPosition.mTick);
    }
    if (flPosition > -kDeadZone && flPosition < kDeadZone) {
        mScratchDirection = kScratchNone;
    }

    mNewestReading = (mNewestReading + 1) % nReadings;
    mReadings[mNewestReading] = flPosition;
}

// NTSC-U/C: 0x001d0038, PAL: 0x001d5ef0
void Scratcher::EraseGemRange(EraseMsg *pMsg) {
    if (pMsg->mTrack != mTrack) {
        return;
    }

    int bErased = 0;
    const int nBar = pMsg->mPosition.mTick / mBarDivisor;
    int nFirstBar;
    int nEndBar;
    if (pMsg->mDoubleTap != 0) {
        nFirstBar = mTrackData->StepStartBar(nBar);
        nEndBar = mTrackData->FollowingStepBar(nFirstBar);
    } else {
        nFirstBar = nBar;
        nEndBar = nBar + 1;
    }

    for (int nClear = nFirstBar; nClear < nEndBar; ++nClear) {
        if (mPhraseMgr->GetPhraseOwner(nClear) != pMsg->mPlayer) {
            continue;
        }
        bErased = 1;
        mPhraseMgr->ClearPhrase(nClear, 0);
        if (nClear == nBar) {
            AllNotesOffMsg allOff;
            Send(&allOff);
        }
    }

    if (bErased == 0) {
        return;
    }
    PlaySoundByName(pMsg->mDoubleTap != 0 ? kEraseStepSound : kEraseBarSound);
    // The effect identifies mPlayer, not the player the message erased for.
    ShowEraseEffectMsg effect(mPlayer, mTrack, nFirstBar, nEndBar, kEraseEffectFlag);
    Send(&effect);
    SendSeekerMsg(pMsg->mPosition.mTick / mBarDivisor);
}

// NTSC-U/C: 0x001d0248, PAL: 0x001d6100
void Scratcher::OnTrackSelect(TrackSelectMsg *pMsg) {
    if (pMsg->mTrack != mTrack) {
        return;
    }
    if (pMsg->mPlayer->IsNull() == 0) {
        NowBarMsg nowBar;
        nowBar.mTrack = mTrack;
        nowBar.mPlayer = pMsg->mPlayer;
        nowBar.mLane = kCenterLane;
        Send(&nowBar);
    }
    if (pMsg->mPlace != 0) {
        return;
    }
    mPlayer = pMsg->mPlayer;
    if (mPlayer->IsNull() == 0) {
        SendSeekerMsg(pMsg->mPosition.mTick / mBarDivisor);
    }
}

// NTSC-U/C: 0x001d0358, PAL: 0x001d6210
void Scratcher::OnPitchRiff(int nGem, int nStep, int nTick) {
    const int nBar = nTick / mBarDivisor;
    const int nLastBar = mLastScratchPosition.mTick / Mid::MBT(kBarTicks).mTick;
    if (QueryBar(nBar) == 0 || (nLastBar == nBar && mLastScratchPlayer != mPlayer)) {
        PlaySoundByName(kInactiveSound);
        return;
    }
    if (mLastScratchPosition.mTick == nTick) {
        return;
    }
    mLastScratchPosition.mTick = nTick;
    mLastScratchPlayer = mPlayer;

    Riff *pRiff = mTrackData->GetRiff(nTick, nGem);
    if (pRiff == nullptr) {
        return;
    }
    MultiMuse *pShifted = TransposeMuse(pRiff, nStep);
    {
        MultiMuseMsg muse(pShifted);
        Send(&muse);
    }
    Attachment::ReleaseIfSet(pShifted);

    if (mAnnouncedBar != nBar) {
        mAnnouncedBar = nBar;
        if (mPhraseMgr->GetPhraseOwner(nBar)->IsNull() == 0) {
            mPhraseMgr->ClearPhrase(nBar, 0);
        }
        int nPoints = mTrackData->GetPoints(nBar);
        if (mPlayer->MarkBarScored(nBar) == 0) {
            nPoints = 0;
        }
        {
            BeginPhraseCatchMsg begin(mPlayer, nPoints, mPlayer->GetMultiplier(nBar));
            Send(&begin);
        }
        PhraseCapturedMsg captured(nBar, nBar + 1, nBar, nBar + 1, mTrack, mPlayer, nPoints, 0, 0);
        Send(&captured);
    }

    const Mid::MBT offset(nTick % mBarDivisor);
    mPhraseMgr->AddGem(nGem, nStep, nBar, offset.mTick, mPlayer, 0);

    // A scratch against the last one's direction, less than half a beat after that gem ends,
    // draws its gem from where the last one ended.
    const Mid::MBT limit(ClampTick(mLastGemEnd.mTick + Mid::MBT(kHalfBeatTicks).mTick));
    int bContinues = 0;
    if (nTick < limit.mTick) {
        bContinues = (nStep * mLastStep) < 0;
    }
    int nStart = nTick;
    float flStartBlend = kCenterLane;
    if (bContinues != 0) {
        nStart = mLastGemEnd.mTick;
        flStartBlend = mLastGemEndBlend;
    }
    mLastStep = nStep;
    mLastGemEnd = Mid::MBT(ClampTick(nTick + Mid::MBT(kScratchGemTicks).mTick));
    mLastGemEndBlend = AxeOldGemMaker::BlendForStep(nStep);

    if (nStep != 0) {
        (void)AxeOldGemMaker::NextStripId(); // Yes, the binary discards the new identity.
        DurGemMsg gem;
        gem.mLane = mTrack;
        gem.mStartFrame = nStart;
        gem.mStartBlend = flStartBlend;
        gem.mEndFrame = mLastGemEnd.mTick;
        gem.mEndBlend = mLastGemEndBlend;
        gem.mLive = kScratchDurGemLive;
        gem.mPlayer = mPlayer;
        Send(&gem);
    }
    if (bContinues == 0) {
        {
            GemMsg gem;
            gem.mPosition.mTick = nTick;
            gem.mTrack = mTrack;
            gem.mGem = kScratchGem;
            gem.mPlayer = mPlayer;
            gem.mGhost = 0;
            Send(&gem);
        }

        PitchMsg pitch;
        pitch.mTick = nTick;
        pitch.mTrack = mTrack;
        pitch.mGem = kScratchGem;
        pitch.mPlayer = mPlayer;
        Send(&pitch);
    }
    AxeButtonMsg press(kButtonPressed, kButtonPressed, mPlayer);
    Send(&press);
}

// NTSC-U/C: 0x001d08e0, PAL: 0x001d6798
void Scratcher::SendSeekerMsg(int) {
    if (mPlayer->IsNull() != 0) {
        return;
    }
    SeekerMsg off(mPlayer);
    Send(&off);
}

// NTSC-U/C: 0x001d1bf0, PAL: 0x001d7aa8
Scratcher::~Scratcher() {
}

// NTSC-U/C: 0x001d0980, PAL: 0x001d6838
void Scratcher::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_nPitchRiffMsgType)) {
        PitchRiffMsg *pRiff = static_cast<PitchRiffMsg *>(pMsg);
        if (pRiff->mTrack != mTrack) {
            return;
        }
        if (mPlayer != pRiff->mPlayer) {
            return;
        }
        mLastGem = pRiff->mButton;
        OnPitchRiff(pRiff->mButton, 0, pRiff->mPosition.mTick);
        return;
    }
    if (nType == static_cast<int>(g_nEraseMsgType)) {
        EraseGemRange(static_cast<EraseMsg *>(pMsg));
        return;
    }
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        OnTrackSelect(static_cast<TrackSelectMsg *>(pMsg));
        return;
    }
    if (nType == static_cast<int>(g_nInvalidateSeekerMsgType)) {
        InvalidateSeekerMsg *pInvalidate = static_cast<InvalidateSeekerMsg *>(pMsg);
        if (pInvalidate->mTrack == mTrack) {
            SendSeekerMsg(pInvalidate->mBar);
        }
        return;
    }
    if (nType == static_cast<int>(g_nAxisRegisterMsgType)) {
        PostNowBarMsg(static_cast<AxisRegisterMsg *>(pMsg));
    }
}

// NTSC-U/C: 0x001d1cc8, PAL: 0x001d7b80
void Scratcher::OnPitchRiffMsg(PitchRiffMsg *pMsg) {
    if (pMsg->mTrack != mTrack) {
        return;
    }
    if (mPlayer != pMsg->mPlayer) {
        return;
    }
    mLastGem = pMsg->mButton;
    OnPitchRiff(pMsg->mButton, 0, pMsg->mPosition.mTick);
}

// NTSC-U/C: 0x001d1d18, PAL: 0x001d7bd0
void Scratcher::OnInvalidateSeeker(InvalidateSeekerMsg *pMsg) {
    if (pMsg->mTrack == mTrack) {
        SendSeekerMsg(pMsg->mBar);
    }
}

// NTSC-U/C: 0x001d1d48, PAL: 0x001d7c00
int Scratcher::QueryBar(int nBar) {
    return mTrackData->QueryBar(nBar);
}

// NTSC-U/C: 0x001d1d68, PAL: 0x001d7c20
int Scratcher::Tick(int nElapsedTicks) {
    const int nBar = nElapsedTicks / mBarDivisor;
    SendSeekerMsg(nBar);
    if (mSwitchesBanks != 0 && mTrackData->IsStepStart(nBar) != 0) {
        Application::shared()->GetSynth()->SelectBank(mChannel, mTrackData->FindStepIndex(nBar));
    }
    return 1;
}
