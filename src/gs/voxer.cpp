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

// The value the constructor gives mPhraseBar and mUnknown60.
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

// 0x001d81b8
Voxer::Voxer(PhraseMgr *pPhraseMgr,
             Quantizer *pQuantizer,
             Sch::TickClock *pClock,
             const TrackData *pTrackData)
    : Pitcher(pClock), mPhraseMgr(pPhraseMgr), mQuantizer(pQuantizer), mTrackData(pTrackData),
      mUnknown44(pTrackData->mUnknown04), mUnknown48(pPhraseMgr->mBarTicks),
      mChannel(pTrackData->mChannel), mUnknown50(&g_nullPlayer), mSustaining(0), mPhrase(nullptr),
      mPhraseBar(kNoValue), mUnknown60(kNoValue) {
}

// 0x001d8370
void Voxer::OnPitchRiff(PitchRiffMsg *pMsg) {
    if (pMsg->mUnknown10 != mUnknown44 || pMsg->mUnknown08 != mUnknown50) {
        return;
    }
    mHeldLevels.set(static_cast<unsigned int>(pMsg->mUnknown04) % kLevelBits);
    UpdateSustain(pMsg->mUnknown0c.mTick);
    AxeButtonMsg press(kButtonPressed, 0, mUnknown50);
    Send(&press);
}

// 0x001d8430
void Voxer::OnStopRiff(StopRiffMsg *pMsg) {
    if (pMsg->mUnknown10 != mUnknown44 || pMsg->mPlayer != mUnknown50) {
        return;
    }
    mHeldLevels.reset(static_cast<unsigned int>(pMsg->mUnknown04) % kLevelBits);
    UpdateSustain(pMsg->mPosition.mTick);
    if (!mHeldLevels.any()) {
        AxeButtonMsg release(kButtonReleased, 0, mUnknown50);
        Send(&release);
    }
}

// 0x001d8508
void Voxer::UpdateSustain(int nTick) {
    const int bHeld = mHeldLevels.any();
    if (mSustaining == bHeld) {
        return;
    }
    if (QueryBar(mQuantizer->Quantize(nTick) / mUnknown48) == 0) {
        PlaySoundByName(kInactiveSound);
        return;
    }

    StdMidiMsg sustain(nTick,
                       kControlChange | mChannel,
                       kSustainController,
                       bHeld != 0 ? kSustainHeld : kSustainReleased);
    mSustaining = bHeld;
    StartPhrase(nTick);
    mPhrase->AddMuseMsg(Mid::MBT(nTick % mUnknown48).mTick, &sustain);
    Send(&sustain);
}

// 0x001d8638
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
        if (mPhraseMgr->GetPhraseOwner(nClear) != mUnknown50) {
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
        ShowEraseEffectMsg effect(mUnknown50, mUnknown44, nFirstBar, nEndBar, kEraseEffectFlag);
        Send(&effect);
    }
    OnInvalidateSeeker(nBar);
}

// 0x001d8840
void Voxer::OnTrackSelect(TrackSelectMsg *pMsg) {
    if (pMsg->mUnknown04 != mUnknown44 || pMsg->mUnknown08 != 0) {
        return;
    }

    if (mHeldLevels.any() && mUnknown50->IsNull() == 0) {
        mHeldLevels.reset();
        UpdateSustain(pMsg->mPosition.mTick);
        AxeButtonMsg release(kButtonReleased, 0, mUnknown50);
        Send(&release);
    }

    mUnknown50 = pMsg->mUnknown10;
    if (mUnknown50->IsNull() != 0) {
        return;
    }
    NowBarMsg nowBar;
    nowBar.mUnknown04 = mUnknown44;
    nowBar.mPlayer = mUnknown50;
    nowBar.mLane = kCenterLane;
    Send(&nowBar);
    OnInvalidateSeeker(pMsg->mPosition.mTick / mUnknown48);
}

// 0x001d89d0
void Voxer::StartPhrase(int nTick) {
    const int nBar = nTick / mUnknown48;
    if (nBar == mPhraseBar) {
        return;
    }

    FinishPhrase(mPhraseBar);
    BarStatusMsg status(nBar, mUnknown44, mUnknown50);
    Send(&status);
    mPhrase = new Phrase();
    mPhrase->mPlayer = mUnknown50;
    mPhraseBar = nBar;
    OnErase(nBar, 0, 0);
}

// 0x001d8b00
void Voxer::FinishPhrase(int nBar) {
    if (nBar != mPhraseBar || mPhrase == nullptr) {
        return;
    }

    if (mSustaining != 0) {
        const Mid::MBT lastTick = MakePosition(mUnknown48 - Mid::MBT(1).mTick);
        const Mid::MBT barStart = MakePosition(mUnknown48 * nBar);
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
        const Mid::MBT nextBar = MakePosition(mUnknown48 * (nBar + 1));
        StartPhrase(nextBar.mTick);
        StdMidiMsg hold(nextBar.mTick, kControlChange | mChannel, kSustainController, kSustainHeld);
        mPhrase->AddMuseMsg(Mid::MBT(0).mTick, &hold);
        Send(&hold);
    }
}

// 0x001d8dc8
int Voxer::Tick(int nElapsedTicks) {
    const int nBar = nElapsedTicks / mUnknown48;
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
        mPhrase->AddMuseMsg(Mid::MBT(nElapsedTicks % mUnknown48).mTick, &release);
        Send(&release);
        AxeButtonMsg button(kButtonReleased, 0, mUnknown50);
        Send(&button);
    }
    return 1;
}

// 0x001d8fb0
void Voxer::OnInvalidateSeeker(int) {
    if (mUnknown50->IsNull() != 0) {
        return;
    }
    SeekerMsg off(mUnknown50);
    Send(&off);
}

// 0x001d9050
void Voxer::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_nPitchRiffMsgType)) {
        OnPitchRiff(static_cast<PitchRiffMsg *>(pMsg));
        return;
    }
    if (nType == static_cast<int>(g_nStopRiffMsgType)) {
        OnStopRiff(static_cast<StopRiffMsg *>(pMsg));
        return;
    }
    if (nType == static_cast<int>(g_nEraseMsgType)) {
        EraseMsg *pErase = static_cast<EraseMsg *>(pMsg);
        if (pErase->mUnknown0c != mUnknown44) {
            return;
        }
        if (mUnknown50 != pErase->mUnknown04) {
            return;
        }
        OnErase(pErase->mUnknown08.mTick / mUnknown48, pErase->mUnknown10, 1);
        return;
    }
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        OnTrackSelect(static_cast<TrackSelectMsg *>(pMsg));
        return;
    }
    if (nType == static_cast<int>(g_nInvalidateSeekerMsgType)) {
        InvalidateSeekerMsg *pInvalidate = static_cast<InvalidateSeekerMsg *>(pMsg);
        if (pInvalidate->mUnknown08 == mUnknown44) {
            OnInvalidateSeeker(pInvalidate->mUnknown04);
        }
    }
}

// 0x001d9e40
void Voxer::OnEraseMsg(EraseMsg *pMsg) {
    if (pMsg->mUnknown0c != mUnknown44) {
        return;
    }
    if (mUnknown50 != pMsg->mUnknown04) {
        return;
    }
    OnErase(pMsg->mUnknown08.mTick / mUnknown48, pMsg->mUnknown10, 1);
}

// 0x001d9e98
void Voxer::OnInvalidateSeekerMsg(InvalidateSeekerMsg *pMsg) {
    if (pMsg->mUnknown08 == mUnknown44) {
        OnInvalidateSeeker(pMsg->mUnknown04);
    }
}

// 0x001d9ec8
int Voxer::QueryBar(int nBar) {
    return mTrackData->QueryBar(nBar);
}
