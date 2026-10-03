#include "gs/voxer.h"

#include <algorithm>

#include "app/attachment.h"
#include "app/playsound.h"
#include "game/nullplayer.h"
#include "game/phrase.h"
#include "game/player.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "gs/phrasemgr.h"
#include "msg/axebuttonmsg.h"
#include "msg/barstatusmsg.h"
#include "msg/erasemsg.h"
#include "msg/invalidateseekermsg.h"
#include "msg/nowbarmsg.h"
#include "msg/pitchriffmsg.h"
#include "msg/seekermsg.h"
#include "msg/showeraseeffectmsg.h"
#include "msg/stdmidimsg.h"
#include "msg/stopriffmsg.h"
#include "msg/trackselectmsg.h"

namespace {

// The value the constructor gives mPhraseBar and mUnreadSentinel.
constexpr int kNoValue = -1;

// The sustain controller the Voxer drives, and its two values.
constexpr unsigned char kControlChange = 0xb0;
constexpr unsigned char kSustainController = 46;
constexpr unsigned char kSustainHeld = 0;
constexpr unsigned char kSustainReleased = 127;

// The two button states AxeButtonMsg reports.
constexpr int kButtonReleased = 0;
constexpr int kButtonPressed = 1;

// The word at ShowEraseEffectMsg `+0x14` that OnErase() always sets.
constexpr int kEraseEffectFlag = 1;

// The lane a new player's now bar starts on, the middle of the tunnel.
constexpr float kCenterLane = 0.5f;

constexpr char kInactiveSound[] = "SND_INACTIVE";
constexpr char kEraseStepSound[] = "SND_ERASE_SECTION";
constexpr char kEraseBarSound[] = "SND_ERASE";

// A computed position, clamped to the finite range as the inline Mid::MBT arithmetic does.
inline Mid::MBT MakePosition(int nTick) {
    return Mid::MBT(std::min(std::max(nTick, kMBTMinimum), kMBTMaximum));
}

} // namespace

// NTSC-U/C: 0x001d81b8, PAL: 0x001de098
Voxer::Voxer(PhraseMgr *pPhraseMgr,
             Quantizer *pQuantizer,
             Sch::TickClock *pClock,
             const TrackData *pTrackData)
    : Pitcher(pClock), mPhraseMgr(pPhraseMgr), mQuantizer(pQuantizer), mTrackData(pTrackData),
      mTrack(pTrackData->mIndex), mBarTicks(pPhraseMgr->mBarTicks), mChannel(pTrackData->mChannel),
      mPlayer(&NullPlayer::sInstance), mSustaining(0), mPhrase(nullptr), mPhraseBar(kNoValue),
      mUnreadSentinel(kNoValue) {
}

// NTSC-U/C: 0x001d8370, PAL: 0x001de250
void Voxer::OnPitchRiff(PitchRiffMsg *pMsg) {
    if (pMsg->mTrack != mTrack || pMsg->mPlayer != mPlayer) {
        return;
    }
    mHeldLevels.set(static_cast<unsigned int>(pMsg->mButton) % kLevelBits);
    UpdateSustain(pMsg->mPosition.mTick);
    AxeButtonMsg press(kButtonPressed, 0, mPlayer);
    Send(&press);
}

// NTSC-U/C: 0x001d8430, PAL: 0x001de310
void Voxer::OnStopRiff(StopRiffMsg *pMsg) {
    if (pMsg->mTrack != mTrack || pMsg->mPlayer != mPlayer) {
        return;
    }
    mHeldLevels.reset(static_cast<unsigned int>(pMsg->mButton) % kLevelBits);
    UpdateSustain(pMsg->mPosition.mTick);
    if (!mHeldLevels.any()) {
        AxeButtonMsg release(kButtonReleased, 0, mPlayer);
        Send(&release);
    }
}

// NTSC-U/C: 0x001d8508, PAL: 0x001de3e8
void Voxer::UpdateSustain(int nTick) {
    const int bHeld = mHeldLevels.any();
    if (mSustaining == bHeld) {
        return;
    }
    if (QueryBar(mQuantizer->Quantize(nTick) / mBarTicks) == 0) {
        PlaySoundByName(kInactiveSound);
        return;
    }

    StdMidiMsg sustain(nTick,
                       kControlChange | mChannel,
                       kSustainController,
                       bHeld != 0 ? kSustainHeld : kSustainReleased);
    mSustaining = bHeld;
    StartPhrase(nTick);
    mPhrase->AddMuseMsg(Mid::MBT(nTick % mBarTicks).mTick, &sustain);
    Send(&sustain);
}

// NTSC-U/C: 0x001d8638, PAL: 0x001de518
void Voxer::OnErase(int nBar, int bWholeStep, int bAnnounce) {
    int bErased = 0;
    int nFirstBar;
    int nEndBar;
    if (bWholeStep != 0) {
        nFirstBar = mTrackData->StepStartBar(nBar);
        nEndBar = mTrackData->FollowingStepBar(nFirstBar);
    } else {
        nFirstBar = nBar;
        nEndBar = nBar + 1;
    }

    for (int nClear = nFirstBar; nClear < nEndBar; ++nClear) {
        if (mPhraseMgr->GetOwner(nClear) != mPlayer) {
            continue;
        }
        bErased = 1;
        mPhraseMgr->ClearPhrase(nClear, 0);
        if (nClear == nBar) {
            StdMidiMsg release(
                kMBTInfinity, kControlChange | mChannel, kSustainController, kSustainReleased);
            Send(&release);
        }
    }

    if (bErased == 0) {
        return;
    }
    if (bAnnounce != 0) {
        PlaySoundByName(bWholeStep != 0 ? kEraseStepSound : kEraseBarSound);
        ShowEraseEffectMsg effect(mPlayer, mTrack, nFirstBar, nEndBar, kEraseEffectFlag);
        Send(&effect);
    }
    OnInvalidateSeeker(nBar);
}

// NTSC-U/C: 0x001d8840, PAL: 0x001de720
void Voxer::OnTrackSelect(TrackSelectMsg *pMsg) {
    if (pMsg->mTrack != mTrack || pMsg->mPlace != 0) {
        return;
    }

    if (mHeldLevels.any() && mPlayer->IsNull() == 0) {
        mHeldLevels.reset();
        UpdateSustain(pMsg->mPosition.mTick);
#ifdef VIDEO_STANDARD_PAL
        mSustaining = 0;
        StdMidiMsg sustainRelease(
            pMsg->mPosition.mTick, kControlChange | mChannel, kSustainController, kSustainReleased);
        Send(&sustainRelease);
#endif
        AxeButtonMsg release(kButtonReleased, 0, mPlayer);
        Send(&release);
    }

    mPlayer = pMsg->mPlayer;
    if (mPlayer->IsNull() != 0) {
        return;
    }
    NowBarMsg nowBar;
    nowBar.mTrack = mTrack;
    nowBar.mPlayer = mPlayer;
    nowBar.mLane = kCenterLane;
    Send(&nowBar);
    OnInvalidateSeeker(pMsg->mPosition.mTick / mBarTicks);
}

// NTSC-U/C: 0x001d89d0, PAL: 0x001de920
void Voxer::StartPhrase(int nTick) {
    const int nBar = nTick / mBarTicks;
    if (nBar == mPhraseBar) {
        return;
    }

    FinishPhrase(mPhraseBar);
    BarStatusMsg status(nBar, mTrack, mPlayer);
    Send(&status);
    mPhrase = new Phrase();
    mPhrase->mPlayer = mPlayer;
    mPhraseBar = nBar;
    OnErase(nBar, 0, 0);
}

// NTSC-U/C: 0x001d8b00, PAL: 0x001dea50
void Voxer::FinishPhrase(int nBar) {
    if (nBar != mPhraseBar || mPhrase == nullptr) {
        return;
    }

    if (mSustaining != 0) {
        const Mid::MBT lastTick = MakePosition(mBarTicks - Mid::MBT(1).mTick);
        const Mid::MBT barStart = MakePosition(mBarTicks * nBar);
        const Mid::MBT when = MakePosition(lastTick.mTick + barStart.mTick);
        StdMidiMsg release(
            when.mTick, kControlChange | mChannel, kSustainController, kSustainReleased);
        mPhrase->AddMuseMsg(lastTick.mTick, &release);
        Send(&release);
    }

    mPhraseMgr->InstallPhrase(mPhrase, mPhraseBar, 0);
    Attachment::ReleaseIfSet(mPhrase);
    mPhrase = nullptr;

    if (mSustaining != 0) {
        const Mid::MBT nextBar = MakePosition(mBarTicks * (nBar + 1));
        StartPhrase(nextBar.mTick);
        StdMidiMsg hold(nextBar.mTick, kControlChange | mChannel, kSustainController, kSustainHeld);
        mPhrase->AddMuseMsg(Mid::MBT(0).mTick, &hold);
        Send(&hold);
    }
}

// NTSC-U/C: 0x001d8dc8, PAL: 0x001ded18
int Voxer::Tick(int nElapsedTicks) {
    const int nBar = nElapsedTicks / mBarTicks;
    if (nBar == 0) {
        StdMidiMsg release(
            kMBTInfinity, kControlChange | mChannel, kSustainController, kSustainReleased);
        Send(&release);
    }
    FinishPhrase(nBar - 1);
    OnInvalidateSeeker(nBar);

    if (mTrackData->QueryBar(nBar) == 0 && mSustaining != 0) {
        StdMidiMsg release(
            nElapsedTicks, kControlChange | mChannel, kSustainController, kSustainReleased);
        mHeldLevels.reset();
        mSustaining = 0;
        StartPhrase(nElapsedTicks);
        mPhrase->AddMuseMsg(Mid::MBT(nElapsedTicks % mBarTicks).mTick, &release);
        Send(&release);
#ifdef VIDEO_STANDARD_PAL
        if (mPlayer->IsNull() == 0) {
            AxeButtonMsg button(kButtonReleased, 0, mPlayer);
            Send(&button);
        }
#else
        AxeButtonMsg button(kButtonReleased, 0, mPlayer);
        Send(&button);
#endif
    }
    return 1;
}

// NTSC-U/C: 0x001d8fb0, PAL: 0x001def20
void Voxer::OnInvalidateSeeker(int) {
    if (mPlayer->IsNull() != 0) {
        return;
    }
    SeekerMsg off(mPlayer);
    Send(&off);
}

// NTSC-U/C: 0x001d9050, PAL: 0x001defc0
void Voxer::DispatchPriv(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(PitchRiffMsg::sID)) {
        OnPitchRiff(static_cast<PitchRiffMsg *>(pMsg));
        return;
    }
    if (nType == static_cast<int>(StopRiffMsg::sID)) {
        OnStopRiff(static_cast<StopRiffMsg *>(pMsg));
        return;
    }
    if (nType == static_cast<int>(EraseMsg::sID)) {
        EraseMsg *pErase = static_cast<EraseMsg *>(pMsg);
        if (pErase->mTrack != mTrack) {
            return;
        }
        if (mPlayer != pErase->mPlayer) {
            return;
        }
        OnErase(pErase->mPosition.mTick / mBarTicks, pErase->mDoubleTap, 1);
        return;
    }
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        OnTrackSelect(static_cast<TrackSelectMsg *>(pMsg));
        return;
    }
    if (nType == static_cast<int>(InvalidateSeekerMsg::sID)) {
        InvalidateSeekerMsg *pInvalidate = static_cast<InvalidateSeekerMsg *>(pMsg);
        if (pInvalidate->mTrack == mTrack) {
            OnInvalidateSeeker(pInvalidate->mBar);
        }
    }
}

// NTSC-U/C: 0x001d9dd0, PAL: 0x001dfd40
Voxer::~Voxer() {
}

// NTSC-U/C: 0x001d9e40, PAL: 0x001dfdb0
void Voxer::OnEraseMsg(EraseMsg *pMsg) {
    if (pMsg->mTrack != mTrack) {
        return;
    }
    if (mPlayer != pMsg->mPlayer) {
        return;
    }
    OnErase(pMsg->mPosition.mTick / mBarTicks, pMsg->mDoubleTap, 1);
}

// NTSC-U/C: 0x001d9e98, PAL: 0x001dfe08
void Voxer::OnInvalidateSeekerMsg(InvalidateSeekerMsg *pMsg) {
    if (pMsg->mTrack == mTrack) {
        OnInvalidateSeeker(pMsg->mBar);
    }
}

// NTSC-U/C: 0x001d9ec8, PAL: 0x001dfe38
int Voxer::QueryBar(int nBar) {
    return mTrackData->QueryBar(nBar);
}
