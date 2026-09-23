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

// The value the constructor gives mUnknown6c.
constexpr int kNoValue = -1;

// mUnknown74 starts with this many elements, each built from the int zero.
constexpr int kUnknown74Count = 3;
constexpr int kUnknown74Initial = 0;

// The two configuration codes that decide mUnknown54.
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
constexpr int kScratchDurGemUnknown18 = 1;
constexpr int kScratchGem = 1;

// AxeButtonMsg's state for a press, which a scratch sends in both of its first two words.
constexpr int kButtonPressed = 1;

// Saturates a tick to the finite range, as the inline Mid::MBT arithmetic does.
inline int ClampTick(int nTick) {
    return std::min(std::max(nTick, kMBTMinimum), kMBTMaximum);
}

constexpr char kEraseStepSound[] = "SND_ERASE_SECTION";
constexpr char kEraseBarSound[] = "SND_ERASE";

} // namespace

// 0x001cf988
Scratcher::Scratcher(PhraseMgr *pPhraseMgr,
                     Quantizer *pQuantizer,
                     Sch::TickClock *pClock,
                     const TrackData *pTrackData)
    : Pitcher(pClock), mPhraseMgr(pPhraseMgr), mQuantizer(pQuantizer), mTrackData(pTrackData),
      mUnknown44(pTrackData->mUnknown04), mBarDivisor(pPhraseMgr->mBarTicks), mClock(pClock),
      mUnknown50(kIDableUnregistered), mUnknown5c(&g_nullPlayer), mUnknown64(&g_nullPlayer),
      mUnknown68(0), mUnknown6c(kNoValue), mUnknown70(0),
      mUnknown74(kUnknown74Count, kUnknown74Initial), mUnknown84(0), mUnknown88(0.0f),
      mUnknown8c(0) {
    mUnknown54 = 0;
    if (QueryConfigFlag(kBankSwitchConfigCode) != 0) {
        mUnknown54 = QueryConfigFlag(kBankSwitchOverrideConfigCode) == 0;
    }
    mUnknown58 = pTrackData->mChannel;
}

// 0x001d0038
void Scratcher::EraseGemRange(EraseMsg *pMsg) {
    if (pMsg->mUnknown0c != mUnknown44) {
        return;
    }

    int bErased = 0;
    const int nBar = pMsg->mUnknown08.mTick / mBarDivisor;
    int nFirstBar;
    int nEndBar;
    if (pMsg->mUnknown10 != 0) {
        nFirstBar = mTrackData->StepStartBar(nBar);
        nEndBar = mTrackData->FollowingStepBar(nFirstBar);
    } else {
        nFirstBar = nBar;
        nEndBar = nBar + 1;
    }

    for (int nClear = nFirstBar; nClear < nEndBar; ++nClear) {
        if (mPhraseMgr->GetPhraseOwner(nClear) != pMsg->mUnknown04) {
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
    PlaySoundByName(pMsg->mUnknown10 != 0 ? kEraseStepSound : kEraseBarSound);
    // The effect names mUnknown5c, not the player the message erased for.
    ShowEraseEffectMsg effect(mUnknown5c, mUnknown44, nFirstBar, nEndBar, kEraseEffectFlag);
    Send(&effect);
    SendSeekerMsg(pMsg->mUnknown08.mTick / mBarDivisor);
}

// 0x001d0248
void Scratcher::OnTrackSelect(TrackSelectMsg *pMsg) {
    if (pMsg->mUnknown04 != mUnknown44) {
        return;
    }
    if (pMsg->mUnknown10->IsNull() == 0) {
        NowBarMsg nowBar;
        nowBar.mUnknown04 = mUnknown44;
        nowBar.mPlayer = pMsg->mUnknown10;
        nowBar.mLane = kCenterLane;
        Send(&nowBar);
    }
    if (pMsg->mUnknown08 != 0) {
        return;
    }
    mUnknown5c = pMsg->mUnknown10;
    if (mUnknown5c->IsNull() == 0) {
        SendSeekerMsg(pMsg->mPosition.mTick / mBarDivisor);
    }
}

// 0x001d0358
void Scratcher::OnPitchRiff(int nGem, int nStep, int nTick) {
    const int nBar = nTick / mBarDivisor;
    const int nLastBar = mUnknown60.mTick / Mid::MBT(kBarTicks).mTick;
    if (QueryBar(nBar) == 0 || (nLastBar == nBar && mUnknown64 != mUnknown5c)) {
        PlaySoundByName(kInactiveSound);
        return;
    }
    if (mUnknown60.mTick == nTick) {
        return;
    }
    mUnknown60.mTick = nTick;
    mUnknown64 = mUnknown5c;

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

    if (mUnknown6c != nBar) {
        mUnknown6c = nBar;
        if (mPhraseMgr->GetPhraseOwner(nBar)->IsNull() == 0) {
            mPhraseMgr->ClearPhrase(nBar, 0);
        }
        int nPoints = mTrackData->GetPoints(nBar);
        if (mUnknown5c->Slot20(nBar) == 0) {
            nPoints = 0;
        }
        BeginPhraseCatchMsg begin(mUnknown5c, nPoints, mUnknown5c->Slot16(nBar));
        Send(&begin);
        PhraseCapturedMsg captured(
            nBar, nBar + 1, nBar, nBar + 1, mUnknown44, mUnknown5c, nPoints, 0, 0);
        Send(&captured);
    }

    const Mid::MBT offset(nTick % mBarDivisor);
    mPhraseMgr->AddGem(nGem, nStep, nBar, offset.mTick, mUnknown5c, 0);

    // A scratch against the last one's direction, less than half a beat after that gem ends,
    // draws its gem from where the last one ended.
    const Mid::MBT limit(ClampTick(mUnknown84.mTick + Mid::MBT(kHalfBeatTicks).mTick));
    int bContinues = 0;
    if (nTick < limit.mTick) {
        bContinues = (nStep * mUnknown8c) < 0;
    }
    int nStart = nTick;
    float flStartBlend = kCenterLane;
    if (bContinues != 0) {
        nStart = mUnknown84.mTick;
        flStartBlend = mUnknown88;
    }
    mUnknown8c = nStep;
    mUnknown84 = Mid::MBT(ClampTick(nTick + Mid::MBT(kScratchGemTicks).mTick));
    mUnknown88 = AxeOldGemMaker::BlendForStep(nStep);

    if (nStep != 0) {
        (void)AxeOldGemMaker::NextStripId(); // Yes, the binary discards the new identity.
        DurGemMsg gem;
        gem.mLane = mUnknown44;
        gem.mStartFrame = nStart;
        gem.mStartBlend = flStartBlend;
        gem.mEndFrame = mUnknown84.mTick;
        gem.mEndBlend = mUnknown88;
        gem.mUnknown18 = kScratchDurGemUnknown18;
        gem.mPlayer = mUnknown5c;
        Send(&gem);
    }
    if (bContinues == 0) {
        GemMsg gem;
        gem.mPosition.mTick = nTick;
        gem.mTrack = mUnknown44;
        gem.mGem = kScratchGem;
        gem.mPlayer = mUnknown5c;
        gem.mGhost = 0;
        Send(&gem);

        PitchMsg pitch;
        pitch.mUnknown04 = nTick;
        pitch.mUnknown08 = mUnknown44;
        pitch.mUnknown0c = kScratchGem;
        pitch.mUnknown10 = mUnknown5c;
        Send(&pitch);
    }
    AxeButtonMsg press(kButtonPressed, kButtonPressed, mUnknown5c);
    Send(&press);
}

// 0x001d08e0
void Scratcher::SendSeekerMsg(int) {
    if (mUnknown5c->IsNull() != 0) {
        return;
    }
    SeekerMsg off(mUnknown5c);
    Send(&off);
}

// 0x001d1bf0
Scratcher::~Scratcher() {
}

// 0x001d0980
void Scratcher::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_nPitchRiffMsgType)) {
        PitchRiffMsg *pRiff = static_cast<PitchRiffMsg *>(pMsg);
        if (pRiff->mUnknown10 != mUnknown44) {
            return;
        }
        if (mUnknown5c != pRiff->mUnknown08) {
            return;
        }
        mUnknown68 = pRiff->mUnknown04;
        OnPitchRiff(pRiff->mUnknown04, 0, pRiff->mUnknown0c.mTick);
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
        if (pInvalidate->mUnknown08 == mUnknown44) {
            SendSeekerMsg(pInvalidate->mUnknown04);
        }
        return;
    }
    if (nType == static_cast<int>(g_nAxisRegisterMsgType)) {
        PostNowBarMsg(static_cast<AxisRegisterMsg *>(pMsg));
    }
}

// 0x001d1cc8
void Scratcher::OnPitchRiffMsg(PitchRiffMsg *pMsg) {
    if (pMsg->mUnknown10 != mUnknown44) {
        return;
    }
    if (mUnknown5c != pMsg->mUnknown08) {
        return;
    }
    mUnknown68 = pMsg->mUnknown04;
    OnPitchRiff(pMsg->mUnknown04, 0, pMsg->mUnknown0c.mTick);
}

// 0x001d1d18
void Scratcher::OnInvalidateSeeker(InvalidateSeekerMsg *pMsg) {
    if (pMsg->mUnknown08 == mUnknown44) {
        SendSeekerMsg(pMsg->mUnknown04);
    }
}

// 0x001d1d48
int Scratcher::QueryBar(int nBar) {
    return mTrackData->QueryBar(nBar);
}

// 0x001d1d68
int Scratcher::Tick(int nElapsedTicks) {
    const int nBar = nElapsedTicks / mBarDivisor;
    SendSeekerMsg(nBar);
    if (mUnknown54 != 0 && mTrackData->IsStepStart(nBar) != 0) {
        Application::shared()->GetSynth()->SelectBank(mUnknown58, mTrackData->FindStepIndex(nBar));
    }
    return 1;
}
