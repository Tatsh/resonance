#include "gs/scratcher.h"

#include "app/application.h"
#include "app/playsound.h"
#include "game/idablebase.h"
#include "game/nullplayer.h"
#include "game/player.h"
#include "game/trackdata.h"
#include "gs/phrasemgr.h"
#include "msg/allnotesoffmsg.h"
#include "msg/axisregistermsg.h"
#include "msg/erasemsg.h"
#include "msg/invalidateseekermsg.h"
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
      mUnknown74(kUnknown74Count, kUnknown74Initial), mUnknown84(0), mUnknown88(0), mUnknown8c(0) {
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
