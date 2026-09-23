#include "gs/notepitcher.h"

#include <algorithm>

#include "app/playsound.h"
#include "game/nullplayer.h"
#include "game/player.h"
#include "game/quantizer.h"
#include "game/riff.h"
#include "game/trackdata.h"
#include "gs/phrasemgr.h"
#include "msg/allnotesoffmsg.h"
#include "msg/erasemsg.h"
#include "msg/eraseoffmsg.h"
#include "msg/invalidateseekermsg.h"
#include "msg/multimusemsg.h"
#include "msg/phrasecapturedmsg.h"
#include "msg/pitchmsg.h"
#include "msg/pitchriffmsg.h"
#include "msg/seekermsg.h"
#include "msg/showeraseeffectmsg.h"
#include "msg/trackselectmsg.h"

namespace {

// The value the constructor gives mUnknown48 and mUnknown54.
constexpr int kNoValue = -1;

// The value the constructor gives mUnknown5c.
constexpr int kUnknown5cInitial = 2;

// PostSeekerMsgSecond() searches this many bars for one the player may play.
constexpr int kSeekerSearchBars = 8;

// The seeker a found bar posts is on.
constexpr int kSeekerOn = 1;

// PostPhraseCapturedMsg() adds every gem with this transposition and this final flag.
constexpr int kGemTrans = 0;
constexpr int kGemFlag = 1;

// A bar's PhraseCapturedMsg carries no juice and does not extend a streak.
constexpr int kNoJuice = 0;
constexpr int kNoStreak = 0;

constexpr char kInactiveSound[] = "SND_INACTIVE";
constexpr char kEraseStepSound[] = "SND_ERASE_SECTION";
constexpr char kEraseBarSound[] = "SND_ERASE";

// A computed position, clamped to the finite range as the inline Mid::MBT arithmetic does.
inline Mid::MBT MakePosition(int nTick) {
    return Mid::MBT(std::min(std::max(nTick, kMBTMinimum), kMBTMaximum));
}

} // namespace

// 0x001b1ce0
NotePitcher::NotePitcher(PhraseMgr *pPhraseMgr,
                         Quantizer *pQuantizer,
                         Sch::TickClock *pClock,
                         const TrackData *pTrackData,
                         int bPlayModeOne,
                         int nUnknown1,
                         int nUnknown2)
    : Pitcher(pClock), mPhraseMgr(pPhraseMgr), mQuantizer(pQuantizer),
      mUnknown40(pTrackData->mUnknown04), mUnknown44(&g_nullPlayer), mUnknown48(kNoValue),
      mBarDivisor(kMBTInfinity), mUnknown54(kNoValue), mPlayModeOne(bPlayModeOne),
      mUnknown5c(kUnknown5cInitial), mUnknown60(nUnknown1), mUnknown64(nUnknown2),
      mTrackData(pTrackData), mClock(pClock) {
    // The divisor is stored twice, the placeholder and then the bar length.
    mBarDivisor = mPhraseMgr->mBarTicks;
}

// 0x001b1f10
void NotePitcher::PostPitchMsg(PitchRiffMsg *pMsg) {
    if (pMsg->mUnknown10 != mUnknown40 || pMsg->mUnknown08 != mUnknown44) {
        return;
    }

    const int nTick = mQuantizer->Quantize(pMsg->mUnknown0c.mTick);
    if (CanPlayBar(nTick / mBarDivisor, mUnknown54) == 0) {
        PlaySoundByName(kInactiveSound);
        return;
    }
    if (IsOtherTick(nTick) != 1) {
        return;
    }

    const int nGem = pMsg->mUnknown04;
    Riff *pRiff = mTrackData->GetRiff(nTick, nGem);
    if (pRiff == nullptr) {
        return;
    }
    MultiMuseMsg muse(pRiff);
    Send(&muse);
    PostPhraseCapturedMsg(nGem, nTick);

    PitchMsg pitch;
    pitch.mUnknown04 = nTick;
    pitch.mUnknown08 = mUnknown40;
    pitch.mUnknown0c = nGem;
    pitch.mUnknown10 = mUnknown44;
    Send(&pitch);
    // The position is stored without the finiteness check.
    mUnknown4c.mTick = nTick;
}

// 0x001b20b0
void NotePitcher::PostAllNotesOffMsg(EraseMsg *pMsg) {
    if (pMsg->mUnknown0c != mUnknown40 || pMsg->mUnknown04 != mUnknown44 || mPlayModeOne != 0) {
        return;
    }

    int bErased = 0;
    const int nBar = pMsg->mUnknown08.mTick / mBarDivisor;
    const int bWholeStep = pMsg->mUnknown10 != 0;
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
        if (mPhraseMgr->GetPhraseOwner(nClear) != mUnknown44) {
            continue;
        }
        bErased = 1;
        mPhraseMgr->ClearPhrase(nClear, 0);
        if (nClear == nBar) {
            AllNotesOffMsg allOff;
            Send(&allOff);
        }
    }

    if (bErased != 0) {
        PlaySoundByName(bWholeStep != 0 ? kEraseStepSound : kEraseBarSound);
        ShowEraseEffectMsg effect(mUnknown44, mUnknown40, nFirstBar, nEndBar, bWholeStep);
        Send(&effect);
        PostSeekerMsgSecond(pMsg->mUnknown08.mTick / mBarDivisor, 0);
    }
    mUnknown48 = pMsg->mUnknown08;
}

// 0x001b22f0
void NotePitcher::PostSeekerMsg(TrackSelectMsg *pMsg) {
    if (pMsg->mUnknown04 != mUnknown40 || pMsg->mUnknown08 != 0) {
        return;
    }

    if (pMsg->mUnknown10->IsNull() != 0) {
        SeekerMsg off(mUnknown44);
        Send(&off);
    }
    mUnknown44 = pMsg->mUnknown10;
    if (pMsg->mUnknown10->IsNull() == 0) {
        PostSeekerMsgSecond(pMsg->mPosition.mTick / mBarDivisor, 1);
    }
}

// 0x001b2400
void NotePitcher::PostPhraseCapturedMsg(int nGem, int nTick) {
    const int nBar = nTick / mBarDivisor;
    if (mUnknown54 != nBar) {
        mUnknown54 = nBar;
        if (mPlayModeOne != 0 && mPhraseMgr->GetPhraseOwner(nBar)->IsNull() == 0) {
            mPhraseMgr->ClearPhrase(nBar, 0);
        }
        PhraseCapturedMsg captured(mUnknown54,
                                   mUnknown54 + 1,
                                   mUnknown54,
                                   mUnknown54 + 1,
                                   mUnknown40,
                                   mUnknown44,
                                   mTrackData->GetPoints(nBar),
                                   kNoJuice,
                                   kNoStreak);
        Send(&captured);
    }

    const Mid::MBT offset(nTick % mBarDivisor);
    if (mUnknown44->Slot10() == 0) {
        (void)CanPlayBar(nBar, mUnknown54); // Yes, the binary discards this call's result.
        mPhraseMgr->AddGem(nGem, kGemTrans, mUnknown54, offset.mTick, mUnknown44, kGemFlag);
    } else {
        const int nFirstBar = mTrackData->StepStartBar(mUnknown54);
        const int nEndBar = mTrackData->FollowingStepBar(mUnknown54);
        for (int nOther = nFirstBar + ((mUnknown54 - nFirstBar) % mUnknown5c); nOther < nEndBar;
             nOther += mUnknown5c) {
            if (nOther != mUnknown54 && CanPlayBar(nOther, nOther) != 0 &&
                mPhraseMgr->PhrasesMatch(mUnknown54, nOther) != 0) {
                mPhraseMgr->AddGem(nGem, kGemTrans, nOther, offset.mTick, mUnknown44, kGemFlag);
            }
        }
        if (CanPlayBar(mUnknown54, mUnknown54) != 0) {
            mPhraseMgr->AddGem(nGem, kGemTrans, mUnknown54, offset.mTick, mUnknown44, kGemFlag);
        }
    }

    mPhraseMgr->ReplayBar(mUnknown54, MakePosition(offset.mTick + Mid::MBT(1).mTick).mTick);
}

// 0x001b2710
void NotePitcher::PostSeekerMsgSecond(int nBar, int bForce) {
    if (mUnknown44->IsNull() != 0) {
        return;
    }
    if (bForce == 0 && mUnknown44->Slot5() != 0) {
        SeekerMsg off(mUnknown44);
        Send(&off);
        return;
    }
    if (mPlayModeOne != 0) {
        return;
    }

    nBar = std::max(nBar, 0);
    if (mUnknown44->Slot10() == 0) {
        SeekerMsg off(mUnknown44);
        Send(&off);
        return;
    }

    for (int nSeek = nBar; nSeek < nBar + kSeekerSearchBars; ++nSeek) {
        if (CanPlayBar(nSeek, mUnknown54) != 0) {
            const int nStepBars = mUnknown5c;
            SeekerMsg on(mUnknown44,
                         (nSeek / nStepBars) * nStepBars,
                         nStepBars,
                         mUnknown40,
                         kSeekerOn,
                         Mid::MBT(0));
            Send(&on);
            return;
        }
    }

    SeekerMsg off(mUnknown44);
    Send(&off);
}

// 0x001b39c0
NotePitcher::~NotePitcher() {
}

// 0x001b3b98
int NotePitcher::Tick(int nElapsedTicks) {
    PostSeekerMsgSecond(nElapsedTicks / mBarDivisor, 0);
    return 1;
}

// 0x001b3bd0
void NotePitcher::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_nPitchRiffMsgType)) {
        PostPitchMsg(static_cast<PitchRiffMsg *>(pMsg));
        return;
    }
    if (nType == static_cast<int>(g_nEraseMsgType)) {
        PostAllNotesOffMsg(static_cast<EraseMsg *>(pMsg));
        return;
    }
    if (nType == static_cast<int>(g_nEraseOffMsgType)) {
        return;
    }
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        PostSeekerMsg(static_cast<TrackSelectMsg *>(pMsg));
        return;
    }
    if (nType == static_cast<int>(g_nInvalidateSeekerMsgType)) {
        InvalidateSeekerMsg *pInvalidate = static_cast<InvalidateSeekerMsg *>(pMsg);
        if (pInvalidate->mUnknown08 == mUnknown40) {
            PostSeekerMsgSecond(pInvalidate->mUnknown04, 0);
        }
    }
}

// 0x001b3a38
void NotePitcher::OnInvalidateSeeker(InvalidateSeekerMsg *pMsg) {
    if (pMsg->mUnknown08 == mUnknown40) {
        PostSeekerMsgSecond(pMsg->mUnknown04, 0);
    }
}

// 0x001b3a68
int NotePitcher::CanPlayBar(int nBar, int nCurrentBar) {
    if (mPlayModeOne != 0) {
        int bPlayable = 0;
        if (mTrackData->QueryBar(nBar) != 0) {
            bPlayable = mUnknown44->Slot9(nBar) != 0;
        }
        return bPlayable;
    }

    Player *pOwner = mPhraseMgr->GetPhraseOwner(nBar);
    int bPlayable = mTrackData->QueryBar(nBar);
    if (mUnknown60 == 0) {
        bPlayable = bPlayable != 0 && (pOwner->IsNull() != 0 || nCurrentBar == nBar);
    }
    if (pOwner->IsNull() != 0) {
        return bPlayable;
    }
    return bPlayable != 0 && pOwner == mUnknown44;
}

// 0x001b3b88
int NotePitcher::IsOtherTick(int nTick) {
    return mUnknown4c.mTick != nTick;
}
