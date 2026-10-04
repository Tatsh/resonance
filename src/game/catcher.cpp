#include "game/catcher.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include "app/application.h"
#include "app/playsound.h"
#include "game/gamemanagerimpl.h"
#include "game/nullplayer.h"
#include "game/riff.h"
#include "mid/tick.h"
#include "msg/beginphrasecatchmsg.h"
#include "msg/catchmsg.h"
#include "msg/catchprogresspacket.h"
#include "msg/caughtbarmsg.h"
#include "msg/gemmsg.h"
#include "msg/multimusemsg.h"
#include "msg/phrasemuffedmsg.h"
#include "msg/seekermsg.h"
#include "sch/command.h"

namespace {

// Start() searches for the first gem from the position before the song starts.
constexpr int kBeforeSongStart = -1;

// The handle value of a command the clock has not queued yet.
constexpr int kUnallocatedCommand = -2;

// The position the constructor gives mLastCaughtPosition and mLastMissedPosition.
constexpr int kNoPosition = -1;

// The bar the constructor gives mLastMuffedBar.
constexpr int kNoBar = -1;

// The value the constructor gives mEnabled.
constexpr int kEnabledInitially = 1;

// Player::GetInputSlot() reports this for a player without a slot.
constexpr int kNoPlayerSlot = -1;

// One bar and one beat at 480 ticks per quarter note.
constexpr int kBarTicks = 1920;
constexpr int kBeatTicks = 480;

// The bars UpdateSeeker() scans for a free bar.
constexpr int kSeekerScanBars = 32;

// The seeker states PostSeekerRangeMsg() records and sends.
constexpr int kSeekerOn = 1;

// OnAutoCatch() plays its bar through CapturePhrase() with both flags set, and marks the message
// handled. PostCaughtBarMsg() clears the third.
constexpr int kAutoCatchFlag = 1;
constexpr int kNoAutoCatchFlag = 0;
constexpr int kMessageHandled = 1;

// TrackData::GetGemAt() reports this when no gem sits at the position.
constexpr int kNoGem = -1;

// SimulateRemoteGem() compares rand() modulo this against the remote success rate times this.
constexpr int kRandomScale = 256;

// The words a CatchMsg carries for a hit or a miss, and for no progress through the phrase.
constexpr int kCatchMiss = 0;
constexpr int kCatchHit = 1;
constexpr int kNoProgress = 0;

// The four player slots Player::GetInputSlot() reports, each with a separate miss sound.
constexpr int kPlayerSlot1 = 0;
constexpr int kPlayerSlot2 = 1;
constexpr int kPlayerSlot3 = 2;
constexpr int kPlayerSlot4 = 3;

// A computed position, clamped to the finite range as the inline Sch::Tick arithmetic does.
inline Sch::Tick MakePosition(int nTick) {
    return Sch::Tick(std::min(std::max(nTick, kTickMinimum), kTickMaximum));
}

/**
 * Scheduler command that runs Catcher::ProcessGemCommand() after a gem.
 *
 * The RTTI name string at `0x007e0fc8` places PostGemCmd in the anonymous namespace that g++ 2.9x
 * qualifies by the signature of
 * `Catcher(PhraseMgr *, Quantizer *, const TrackData *, Sch::TickClock *, int, Sch::Tick)`. Its
 * one base is Sch::Command, and its vtable is at `0x007e0998`. Catcher::SchedulePostGemCommand()
 * expands the constructor into its 0x14-byte allocation.
 *
 * The destructor at `0x001b1678` is implicitly declared.
 */
class PostGemCmd : public Sch::Command {
public:
    PostGemCmd(Catcher *pOwner, int nTick) : mOwner(pOwner), mTick(nTick) {
    }

    // NTSC-U/C: 0x001b16f0, PAL: 0x001b74b0
    virtual int CmdID() {
        return sCmdID;
    }

    // NTSC-U/C: 0x001b1700, PAL: 0x001b74c0
    virtual void Execute() {
        mOwner->ProcessGemCommand(mTick);
    }

    // NTSC-U/C: 0x001b1720, PAL: 0x001b74e0
    virtual void Print(std::ostream &stream) {
        stream << "{Catcher/PostGem}";
    }

    // The word at 0x00686a98, which the image initialises to zero.
    static int sCmdID;

private:
    Catcher *mOwner; // +0x0c
    int mTick;       // +0x10
};

int PostGemCmd::sCmdID;

/**
 * Scheduler command that runs Catcher::SimulateRemoteGem() at a gem.
 *
 * The RTTI name string at `0x007e1030` places GemCmd in the anonymous namespace that g++ 2.9x
 * qualifies by the signature of
 * `Catcher(PhraseMgr *, Quantizer *, const TrackData *, Sch::TickClock *, int, Sch::Tick)`. Its
 * one base is Sch::Command, and its vtable is at `0x007e0950`. Catcher::ScheduleGemCommand()
 * expands the constructor into its 0x14-byte allocation.
 *
 * The destructor at `0x001b1750` is implicitly declared.
 */
class GemCmd : public Sch::Command {
public:
    GemCmd(Catcher *pOwner, int nTick) : mOwner(pOwner), mTick(nTick) {
    }

    // NTSC-U/C: 0x001b17c8, PAL: 0x001b7588
    virtual int CmdID() {
        return sCmdID;
    }

    // NTSC-U/C: 0x001b17d8, PAL: 0x001b7598
    virtual void Execute() {
        mOwner->SimulateRemoteGem(mTick);
    }

    // NTSC-U/C: 0x001b17f8, PAL: 0x001b75b8
    virtual void Print(std::ostream &stream) {
        stream << "{Catcher/Gem}";
    }

    // The word at 0x00686aa4, which the image initialises to zero.
    static int sCmdID;

private:
    Catcher *mOwner; // +0x0c
    int mTick;       // +0x10
};

int GemCmd::sCmdID;

} // namespace

Catcher::Catcher(PhraseMgr *pPhraseMgr,
                 Quantizer *pQuantizer,
                 const TrackData *pTrackData,
                 Sch::TickClock *pClock,
                 int nSeekerBarCount,
                 Sch::Tick catchWindow)
    : mQuantizer(pQuantizer), mPhraseMgr(pPhraseMgr), mTrackData(pTrackData),
      mPlayer(&NullPlayer::sInstance), mClock(pClock), mSeekerBarCount(nSeekerBarCount),
      mCatchWindow(catchWindow.mTick), mEnabled(kEnabledInitially),
      mLastCaughtPosition(kNoPosition), mLastMissedPosition(kNoPosition),
      mTrack(pTrackData->mIndex), mCaughtGems(0), mMissedGems(0), mMuffedGems(0), mLastEndedBar(0),
      mPhraseRunBars(0), mLastMuffedBar(kNoBar), mSeekerEnabled(0),
      mRemotePlayer(&NullPlayer::sInstance), mRemoteSuccess(0), mRemotePosition(0) {
    mPostGemCommand.mValue = kUnallocatedCommand;
    mGemCommand.mValue = kUnallocatedCommand;
    mTicksPerBar.mTick = pPhraseMgr->mBarTicks;
}

Catcher::~Catcher() {
    Stop();
}

void Catcher::MissGem(int nTick, int nGem) {
    switch (mPlayer->GetInputSlot()) {
    case kPlayerSlot1:
        PlaySoundByName("SND_MISS_PLAYER1");
        break;
    case kPlayerSlot2:
        PlaySoundByName("SND_MISS_PLAYER2");
        break;
    case kPlayerSlot3:
        PlaySoundByName("SND_MISS_PLAYER3");
        break;
    case kPlayerSlot4:
        PlaySoundByName("SND_MISS_PLAYER4");
        break;
    default:
        break;
    }

    CatchMsg msg(nTick, mTrack, nGem, kCatchMiss, mPlayer, kNoProgress, kNoProgress);
    Send(&msg);

    const int nBar = nTick / mTicksPerBar.mTick;
    if (nBar == mLastEndedBar || mPhraseRunBars > 0) {
        // The tick is stored without the finiteness check.
        mLastMissedPosition.mTick = nTick;
        mPhraseRunBars = 0;
        ++mMissedGems;
        PostPhraseMuffedMsg(nBar, mLastMissedPosition);
        UpdateSeeker(nBar);
    }
}

void Catcher::CatchGem(int nTick, int nGem) {
    // The tick is stored without the finiteness check.
    mLastCaughtPosition.mTick = nTick;
    ++mCaughtGems;

    const int nBar = nTick / mTicksPerBar.mTick;
    const Sch::Tick barStart = MakePosition(mTicksPerBar.mTick * nBar);
    if (mLastMissedPosition.mTick < barStart.mTick) {
        mMissedGems = 0;
    }

    MultiMuseMsg riffMsg(mTrackData->GetRiff(nTick, nGem));
    Send(&riffMsg);

    int nPoints = 0;
    int nCaught = 0;
    int nTotal = 0;
    if (mMissedGems == 0 && mMuffedGems == 0) {
        const int nEndBar = mSeekerEndBar;
        nCaught = mCaughtGems;
        if (!(mTrackData->FollowingStepBar(nBar) < nEndBar)) {
            for (int nPhraseBar = nBar - mPhraseRunBars; nPhraseBar < nEndBar; ++nPhraseBar) {
                const int nGems = static_cast<int>(mTrackData->GetGems(nPhraseBar)->size());
                nPoints += mTrackData->GetPoints(nPhraseBar);
                nTotal += nGems;
                nCaught += (nPhraseBar < nBar) * nGems;
            }
            if (nCaught == 1) {
                BeginPhraseCatchMsg beginMsg(mPlayer, nPoints, mPlayer->GetMultiplier(nBar));
                Send(&beginMsg);
            }
        }
    }

    CatchMsg catchMsg(nTick, mTrack, nGem, kCatchHit, mPlayer, nCaught - 1, nTotal);
    Send(&catchMsg);

    // The position is stored without the finiteness check.
    Sch::Tick position;
    position.mTick = nTick;
    GemMsg gemMsg(position, mTrack, nGem, mPlayer);
    Send(&gemMsg);

    mPlayer->CountCaughtGem(); // Yes, the binary discards this call's result.

    const int nNextBar = FindNextGemTick(nTick) / mTicksPerBar.mTick;
    if (nNextBar != nBar) {
        PostCaughtBarMsg(nBar, nNextBar);
    }
}

int Catcher::SnapToNearestGem(int nTick) {
    Sch::Tick before(kTickMinimum);
    Sch::Tick after(kTickMaximum);
    int nBeforeGem;
    int nAfterGem;
    mTrackData->FindGemAtOrBefore(nTick, &before.mTick, &nBeforeGem);
    mTrackData->FindGemAtOrAfter(nTick, &after.mTick, &nAfterGem);

    const Sch::Tick toBefore = MakePosition(nTick - before.mTick);
    const Sch::Tick toAfter = MakePosition(after.mTick - nTick);
    if (toBefore.mTick < toAfter.mTick) {
        if (!(mCatchWindow < toBefore.mTick)) {
            return before.mTick;
        }
    } else if (!(mCatchWindow < toAfter.mTick)) {
        return after.mTick;
    }
    return nTick;
}

void Catcher::PostCatchMsg(PitchRiffMsg *pMsg) {
    if (pMsg->mTrack != mTrack) {
        return;
    }
    if (pMsg->mPlayer != mPlayer) {
        pMsg->mPlayer->GetPlace(); // Yes, the binary discards this call's result.
        PlaySoundByName("SND_INACTIVE");
        return;
    }

    const int nTick = pMsg->mPosition.mTick;
    const int nSnapped = SnapToNearestGem(nTick);
    const int nGem = pMsg->mButton;
    if (!IsBarFree(nSnapped / mTicksPerBar.mTick)) {
        PlaySoundByName("SND_INACTIVE");
        CatchMsg msg(nTick, mTrack, nGem, kCatchMiss, mPlayer, kNoProgress, kNoProgress);
        Send(&msg);
        return;
    }

    const int nGemAt = mTrackData->GetGemAt(nSnapped);
    if (nGemAt != kNoGem && nGemAt == nGem && mLastCaughtPosition.mTick != nSnapped) {
        CatchGem(nSnapped, nGem);
    } else {
        MissGem(nTick, nGem);
    }
}

void Catcher::OnTrackSelect(TrackSelectMsg *pMsg) {
    if (pMsg->mTrack != mTrack) {
        return;
    }
    if (pMsg->mPlace != 0) {
        return;
    }

    const int nBar = pMsg->mPosition.mTick / mTicksPerBar.mTick;
    Player *pNewPlayer = pMsg->mPlayer;
    if (!mPlayer->IsNull() && mPlayer != pNewPlayer && mPlayer->GetInputSlot() != kNoPlayerSlot) {
        PostSeekerMsg();
    }
    mPlayer = pNewPlayer;
    mLastCaughtPosition = Sch::Tick(kNoPosition);
    mPhraseRunBars = 0;
    mMuffedGems += mCaughtGems;

    if (!mPlayer->IsNull()) {
        mCaughtGems = 0;
        UpdateSeeker(nBar);
        return;
    }
    if (mCaughtGems > 0 || nBar < mLastEndedBar) {
        mLastMuffedBar = nBar;
    }
}

void Catcher::OnAutoCatch(AutoCatchMsg *pMsg) {
    if (pMsg->mTrack != mTrack) {
        return;
    }
    const int nBar = pMsg->mBar;
    if (!IsBarFree(nBar)) {
        return;
    }

    Player *pSavedPlayer = mPlayer;
    mPlayer = pMsg->mPlayer;
    CapturePhrase(nBar, kAutoCatchFlag, kAutoCatchFlag);
    mPlayer = pSavedPlayer;
    pMsg->mResult = kMessageHandled;
    UpdateSeeker(nBar);

    const int nNow = Application::shared()->GetSongClock()->SongTick();
    if (nBar == nNow / Sch::Tick(kBarTicks).mTick) {
        const Sch::Tick offset(nNow % Sch::Tick(kBarTicks).mTick);
        mPhraseMgr->ReplayBar(nBar, offset.mTick);
    }
}

void Catcher::PostCaughtBarMsg(int nBar, int nNextBar) {
    if ((mMissedGems + mMuffedGems) != 0) {
        return;
    }

    ++mPhraseRunBars;
    CaughtBarMsg msg(mPlayer, nBar);
    mPlayer->Dispatch(&msg);
    ReportCaughtPowerbar(nBar);

    int nLastBar = nBar;
    if ((nBar + 1) < nNextBar) {
        nLastBar = std::min(nNextBar - 1, mTrackData->FollowingStepBar(nBar) - 1);
        mPhraseRunBars += nLastBar - nBar;
        if (!(mPhraseRunBars < mSeekerBarCount)) {
            nLastBar -= mPhraseRunBars - mSeekerBarCount;
            mPhraseRunBars = mSeekerBarCount;
        }
    }

    if (mSeekerEnabled != 0 && (nLastBar + 1) == mSeekerEndBar) {
        CapturePhrase(nLastBar, mPhraseRunBars, kNoAutoCatchFlag);
        mPhraseRunBars = 0;
        UpdateSeeker(nLastBar);
    }
}

void Catcher::EndBar(int nBar) {
    if (mTrackData->IsStepStart(nBar)) {
        mPhraseRunBars = 0;
    }

    if ((mMissedGems + mMuffedGems) > 0 || mCaughtGems == 0) {
        mPhraseRunBars = 0;
        if (mCaughtGems > 0) {
            // Beat-sized, not bar-sized. That is what the binary computes.
            const Sch::Tick position = MakePosition(nBar * Sch::Tick(kBeatTicks).mTick);
            PostPhraseMuffedMsg(nBar - 1, position);
        }
    }

    mLastEndedBar = nBar;
    mMuffedGems = 0;
    mCaughtGems = 0;
    mMissedGems = 0;
}

int Catcher::FindNextGemTick(int nTick) {
    Sch::Tick next;
    int nGem;
    const Sch::Tick start = MakePosition(nTick + Sch::Tick(1).mTick);
    if (mTrackData->FindGemAtOrAfter(start.mTick, &next.mTick, &nGem)) {
        return next.mTick;
    }

    const Sch::Tick span =
        MakePosition((mTrackData->mGemSearchBars - 1) * Sch::Tick(kBarTicks).mTick);
    next = MakePosition(nTick + span.mTick);
    return next.mTick;
}

void Catcher::SchedulePostGemCommand(int nTick) {
    const int nGemTick = FindNextGemTick(nTick);
    const int nDelay = PostGemDelay(nGemTick);
    const Sch::Tick when = MakePosition(nDelay + Sch::Tick(1).mTick);

    PostGemCmd *pCommand = new PostGemCmd(this, nGemTick);
    mClock->PostAtSongTick(pCommand, when.mTick, mPostGemCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

void Catcher::ScheduleGemCommand(int nTick) {
    const int nGemTick = FindNextGemTick(nTick);

    GemCmd *pCommand = new GemCmd(this, nGemTick);
    mClock->PostAtSongTick(pCommand, nGemTick, mGemCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

int Catcher::PostGemDelay(int nTick) {
    const int nNext = FindNextGemTick(nTick);
    Sch::Tick delay(MakePosition(nTick + nNext).mTick / 2);
    if (MakePosition(nTick + mCatchWindow).mTick < delay.mTick) {
        delay = MakePosition(nTick + mCatchWindow);
    }
    return delay.mTick;
}

void Catcher::SimulateRemoteGem(int nTick) {
    if (mRemotePlayer != &NullPlayer::sInstance && mRemotePlayer->GetInputSlot() == kNoPlayerSlot) {
        const Sch::Tick window = MakePosition(mRemotePosition.mTick + Sch::Tick(kBarTicks).mTick);
        if (!(window.mTick < nTick) &&
            static_cast<float>(std::rand() % kRandomScale) < mRemoteSuccess * kRandomScale) {
            Sch::Tick gemTick;
            int nGem;
            mTrackData->FindGemAtOrAfter(nTick, &gemTick.mTick, &nGem);

            // The gem value selects the riff level. That is what the binary passes.
            Riff *pRiff = mTrackData->GetRiff(nTick, nGem);
            if (pRiff != nullptr) {
                MultiMuseMsg riffMsg(pRiff);
                Send(&riffMsg);
            }

            CatchMsg catchMsg(
                nTick, mTrack, nGem, kCatchHit, mRemotePlayer, kNoProgress, kNoProgress);
            Send(&catchMsg);

            // The position is stored without the finiteness check.
            Sch::Tick position;
            position.mTick = nTick;
            GemMsg gemMsg(position, mTrack, nGem, mRemotePlayer);
            Send(&gemMsg);
        }
    }
    ScheduleGemCommand(nTick);
}

void Catcher::UpdateSeeker(int nBar) {
    if (mPlayer->IsNull()) {
        return;
    }
    if (mPlayer->GetInputSlot() == kNoPlayerSlot) {
        return;
    }
    if (mPlayer->GetPlace()) {
        PostSeekerMsg();
    }

    const int nStart = (mLastMuffedBar < nBar) ? nBar : (mLastMuffedBar + 1);
    int nStepBar = mTrackData->NextStepBar(nStart);
    for (int nScanBar = nStart; nScanBar < (nStart + kSeekerScanBars); ++nScanBar) {
        if (!IsBarFree(nScanBar)) {
            continue;
        }

        const int nFirstBar = nScanBar - mPhraseRunBars;
        const int nEndBar = nScanBar + mSeekerBarCount;
        while (!(nFirstBar < nStepBar)) {
            nStepBar = mTrackData->FollowingStepBar(nStepBar);
        }

        const Sch::Tick start = MakePosition(Sch::Tick(kBarTicks).mTick * nFirstBar);
        const Sch::Tick beforeStart = MakePosition(start.mTick - Sch::Tick(1).mTick);
        const int nGemTick = FindNextGemTick(beforeStart.mTick);
        const int nGemBar = nGemTick / Sch::Tick(kBarTicks).mTick;

        const bool bFits = (nFirstBar < nGemBar) ?
                               (nEndBar == nStepBar && nGemBar == (nEndBar - 1)) :
                               !(nStepBar < nEndBar);
        if (bFits) {
            PostSeekerRangeMsg(nFirstBar, mSeekerBarCount);
            return;
        }
    }
    PostSeekerMsg();
}

void Catcher::PostPhraseMuffedMsg(int nBar, Sch::Tick position) {
    if (nBar == mLastMuffedBar) {
        return;
    }
    mLastMuffedBar = nBar;
    if (mPlayer->IsNull()) {
        return;
    }

    int nTried = 0;
    if (IsBarFree(nBar)) {
        nTried = (mMissedGems + mCaughtGems) > 0;
    }
    PhraseMuffedMsg msg(mTrack, mPlayer, position, nTried);
    Send(&msg);
}

void Catcher::PostSeekerMsg() {
    SeekerMsg msg(mPlayer);
    Send(&msg);
    mSeekerEnabled = 0;
}

void Catcher::PostSeekerRangeMsg(int nFirstBar, int nBarCount) {
    SeekerMsg msg(mPlayer, nFirstBar, nBarCount, mTrack, kSeekerOn, Sch::Tick(0));
    Send(&msg);
    mSeekerEndBar = nFirstBar + nBarCount;
    mSeekerEnabled = kSeekerOn;
    mSeekerFirstBar = nFirstBar;
}

void Catcher::DispatchPriv(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == PitchRiffMsg::sID) {
        PostCatchMsg(static_cast<PitchRiffMsg *>(pMsg));
    } else if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        OnTrackSelect(static_cast<TrackSelectMsg *>(pMsg));
    } else if (nType == g_nCatchProgressPacketType) {
        CatchProgressPacket *pPacket = static_cast<CatchProgressPacket *>(pMsg);
        if (pPacket->mTrack == mTrack) {
            mRemotePlayer = pPacket->mPlayer;
            mRemoteSuccess = pPacket->mSucc;
            mRemotePosition = pPacket->mPosition;
        }
    } else if (nType == g_nAutoCatchMsgType) {
        OnAutoCatch(static_cast<AutoCatchMsg *>(pMsg));
    } else if (nType == InvalidateSeekerMsg::sID) {
        OnInvalidateSeeker(static_cast<InvalidateSeekerMsg *>(pMsg));
    }
}

int Catcher::IsBarFree(int nBar) {
    if (nBar < 0) {
        return 0;
    }
    if (mTrackData->QueryBar(nBar) == 0) {
        return 0;
    }
    return mPhraseMgr->GetOwner(nBar)->IsNull() != 0;
}

void Catcher::OnInvalidateSeeker(InvalidateSeekerMsg *pMsg) {
    if (pMsg->mTrack == mTrack) {
        UpdateSeeker(pMsg->mBar);
    }
}

void Catcher::Start() {
    SchedulePostGemCommand(Sch::Tick(kBeforeSongStart).mTick);
    if (Application::shared()->GetGameMode() == kGameModeNet) {
        ScheduleGemCommand(Sch::Tick(kBeforeSongStart).mTick);
    }
}

void Catcher::Stop() {
    mClock->Withdraw(mPostGemCommand);
    if (Application::shared()->GetGameMode() == kGameModeNet) {
        mClock->Withdraw(mGemCommand);
    }
}

void Catcher::ProcessGemCommand(int nTick) {
    if (mLastCaughtPosition.mTick != nTick) {
        mPhraseRunBars = 0;
        ++mMuffedGems;

        // The position is passed without the finiteness check.
        Sch::Tick position;
        position.mTick = nTick;
        PostPhraseMuffedMsg(nTick / mTicksPerBar.mTick, position);
        UpdateSeeker(mLastMuffedBar);
        if (mPlayer != nullptr) {
            mPlayer->CountMissedGem();
        }
    }

    const int nBar = nTick / mTicksPerBar.mTick;
    const int nNextBar = FindNextGemTick(nTick) / mTicksPerBar.mTick;
    if (nBar < nNextBar) {
        EndBar(nNextBar);
    }
    SchedulePostGemCommand(nTick);
}

void Catcher::SetPhraseOwners(int nFirstBar, int nEndBar, Player *pPlayer) {
    (void)pPlayer->IsNull(); // Yes, the binary discards this call's result.
    for (int nBar = nFirstBar; nBar < nEndBar; ++nBar) {
        mPhraseMgr->SetPhraseOwner(pPlayer, nBar);
    }
}

int Catcher::IsPhraseRunEmpty() {
    return mPhraseRunBars == 0;
}
