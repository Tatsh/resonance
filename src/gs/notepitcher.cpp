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

// The value the constructor gives mLastErasePosition and mCapturedBar.
constexpr int kNoValue = -1;

// The value the constructor gives mStepBars.
constexpr int kInitialStepBars = 2;

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

// NTSC-U/C: 0x001b1ce0, PAL: 0x001b7ab8
NotePitcher::NotePitcher(PhraseMgr *pPhraseMgr,
                         Quantizer *pQuantizer,
                         Sch::TickClock *pClock,
                         const TrackData *pTrackData,
                         int bPlayModeOne,
                         int bAllowOwnedBars,
                         int nUnreadOption)
    : Pitcher(pClock), mPhraseMgr(pPhraseMgr), mQuantizer(pQuantizer), mTrack(pTrackData->mIndex),
      mPlayer(&g_nullPlayer), mLastErasePosition(kNoValue), mBarDivisor(kMBTInfinity),
      mCapturedBar(kNoValue), mPlayModeOne(bPlayModeOne), mStepBars(kInitialStepBars),
      mAllowOwnedBars(bAllowOwnedBars), mUnreadOption(nUnreadOption), mTrackData(pTrackData),
      mClock(pClock) {
    // The divisor is stored twice, the placeholder and then the bar length.
    mBarDivisor = mPhraseMgr->mBarTicks;
}

// NTSC-U/C: 0x001b1f10, PAL: 0x001b7ce8
void NotePitcher::PostPitchMsg(PitchRiffMsg *pMsg) {
    if (pMsg->mTrack != mTrack || pMsg->mPlayer != mPlayer) {
        return;
    }

    const int nTick = mQuantizer->Quantize(pMsg->mPosition.mTick);
    if (CanPlayBar(nTick / mBarDivisor, mCapturedBar) == 0) {
        PlaySoundByName(kInactiveSound);
        return;
    }
    if (IsOtherTick(nTick) != 1) {
        return;
    }

    const int nGem = pMsg->mButton;
    Riff *pRiff = mTrackData->GetRiff(nTick, nGem);
    if (pRiff == nullptr) {
        return;
    }
    MultiMuseMsg muse(pRiff);
    Send(&muse);
    PostPhraseCapturedMsg(nGem, nTick);

    PitchMsg pitch;
    pitch.mTick = nTick;
    pitch.mTrack = mTrack;
    pitch.mGem = nGem;
    pitch.mPlayer = mPlayer;
    Send(&pitch);
    // The position is stored without the finiteness check.
    mLastPitchPosition.mTick = nTick;
}

// NTSC-U/C: 0x001b20b0, PAL: 0x001b7e88
void NotePitcher::PostAllNotesOffMsg(EraseMsg *pMsg) {
    if (pMsg->mTrack != mTrack || pMsg->mPlayer != mPlayer || mPlayModeOne != 0) {
        return;
    }

    int bErased = 0;
    const int nBar = pMsg->mPosition.mTick / mBarDivisor;
    const int bWholeStep = pMsg->mDoubleTap != 0;
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
        if (mPhraseMgr->GetPhraseOwner(nClear) != mPlayer) {
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
        ShowEraseEffectMsg effect(mPlayer, mTrack, nFirstBar, nEndBar, bWholeStep);
        Send(&effect);
        PostSeekerMsgSecond(pMsg->mPosition.mTick / mBarDivisor, 0);
    }
    mLastErasePosition = pMsg->mPosition;
}

// NTSC-U/C: 0x001b22f0, PAL: 0x001b80c8
void NotePitcher::PostSeekerMsg(TrackSelectMsg *pMsg) {
    if (pMsg->mTrack != mTrack || pMsg->mPlace != 0) {
        return;
    }

    if (pMsg->mPlayer->IsNull() != 0) {
        SeekerMsg off(mPlayer);
        Send(&off);
    }
    mPlayer = pMsg->mPlayer;
    if (pMsg->mPlayer->IsNull() == 0) {
        PostSeekerMsgSecond(pMsg->mPosition.mTick / mBarDivisor, 1);
    }
}

// NTSC-U/C: 0x001b2400, PAL: 0x001b81d8
void NotePitcher::PostPhraseCapturedMsg(int nGem, int nTick) {
    const int nBar = nTick / mBarDivisor;
    if (mCapturedBar != nBar) {
        mCapturedBar = nBar;
        if (mPlayModeOne != 0 && mPhraseMgr->GetPhraseOwner(nBar)->IsNull() == 0) {
            mPhraseMgr->ClearPhrase(nBar, 0);
        }
        PhraseCapturedMsg captured(mCapturedBar,
                                   mCapturedBar + 1,
                                   mCapturedBar,
                                   mCapturedBar + 1,
                                   mTrack,
                                   mPlayer,
                                   mTrackData->GetPoints(nBar),
                                   kNoJuice,
                                   kNoStreak);
        Send(&captured);
    }

    const Mid::MBT offset(nTick % mBarDivisor);
    if (mPlayer->IsLooping() == 0) {
        (void)CanPlayBar(nBar, mCapturedBar); // Yes, the binary discards this call's result.
        mPhraseMgr->AddGem(nGem, kGemTrans, mCapturedBar, offset.mTick, mPlayer, kGemFlag);
    } else {
        const int nFirstBar = mTrackData->StepStartBar(mCapturedBar);
        const int nEndBar = mTrackData->FollowingStepBar(mCapturedBar);
        for (int nOther = nFirstBar + ((mCapturedBar - nFirstBar) % mStepBars); nOther < nEndBar;
             nOther += mStepBars) {
            if (nOther != mCapturedBar && CanPlayBar(nOther, nOther) != 0 &&
                mPhraseMgr->PhrasesMatch(mCapturedBar, nOther) != 0) {
                mPhraseMgr->AddGem(nGem, kGemTrans, nOther, offset.mTick, mPlayer, kGemFlag);
            }
        }
        if (CanPlayBar(mCapturedBar, mCapturedBar) != 0) {
            mPhraseMgr->AddGem(nGem, kGemTrans, mCapturedBar, offset.mTick, mPlayer, kGemFlag);
        }
    }

    mPhraseMgr->ReplayBar(mCapturedBar, MakePosition(offset.mTick + Mid::MBT(1).mTick).mTick);
}

// NTSC-U/C: 0x001b2710, PAL: 0x001b84e8
void NotePitcher::PostSeekerMsgSecond(int nBar, int bForce) {
    if (mPlayer->IsNull() != 0) {
        return;
    }
    if (bForce == 0 && mPlayer->GetPlace() != 0) {
        SeekerMsg off(mPlayer);
        Send(&off);
        return;
    }
    if (mPlayModeOne != 0) {
        return;
    }

    nBar = std::max(nBar, 0);
    if (mPlayer->IsLooping() == 0) {
        SeekerMsg off(mPlayer);
        Send(&off);
        return;
    }

    for (int nSeek = nBar; nSeek < nBar + kSeekerSearchBars; ++nSeek) {
        if (CanPlayBar(nSeek, mCapturedBar) != 0) {
            const int nStepBars = mStepBars;
            SeekerMsg on(mPlayer,
                         (nSeek / nStepBars) * nStepBars,
                         nStepBars,
                         mTrack,
                         kSeekerOn,
                         Mid::MBT(0));
            Send(&on);
            return;
        }
    }

    SeekerMsg off(mPlayer);
    Send(&off);
}

// NTSC-U/C: 0x001b39c0, PAL: 0x001b9798
NotePitcher::~NotePitcher() {
}

// NTSC-U/C: 0x001b3b98, PAL: 0x001b9970
int NotePitcher::Tick(int nElapsedTicks) {
    PostSeekerMsgSecond(nElapsedTicks / mBarDivisor, 0);
    return 1;
}

// NTSC-U/C: 0x001b3bd0, PAL: 0x001b99a8
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
        if (pInvalidate->mTrack == mTrack) {
            PostSeekerMsgSecond(pInvalidate->mBar, 0);
        }
    }
}

// NTSC-U/C: 0x001b3a38, PAL: 0x001b9810
void NotePitcher::OnInvalidateSeeker(InvalidateSeekerMsg *pMsg) {
    if (pMsg->mTrack == mTrack) {
        PostSeekerMsgSecond(pMsg->mBar, 0);
    }
}

// NTSC-U/C: 0x001b3a68, PAL: 0x001b9840
int NotePitcher::CanPlayBar(int nBar, int nCurrentBar) {
    if (mPlayModeOne != 0) {
        int bPlayable = 0;
        if (mTrackData->QueryBar(nBar) != 0) {
            bPlayable = mPlayer->IsFreestyleBar(nBar) != 0;
        }
        return bPlayable;
    }

    Player *pOwner = mPhraseMgr->GetPhraseOwner(nBar);
    int bPlayable = mTrackData->QueryBar(nBar);
    if (mAllowOwnedBars == 0) {
        bPlayable = bPlayable != 0 && (pOwner->IsNull() != 0 || nCurrentBar == nBar);
    }
    if (pOwner->IsNull() != 0) {
        return bPlayable;
    }
    return bPlayable != 0 && pOwner == mPlayer;
}

// NTSC-U/C: 0x001b3b88, PAL: 0x001b9960
int NotePitcher::IsOtherTick(int nTick) {
    return mLastPitchPosition.mTick != nTick;
}
