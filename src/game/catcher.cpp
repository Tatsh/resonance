#include "game/catcher.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include "app/application.h"
#include "app/playsound.h"
#include "game/gamemanagerimpl.h"
#include "game/nullplayer.h"
#include "game/riff.h"
#include "mid/mbt.h"
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

// A computed position, clamped to the finite range as the inline Mid::MBT arithmetic does.
inline Mid::MBT MakePosition(int nTick) {
    return Mid::MBT(std::min(std::max(nTick, kMBTMinimum), kMBTMaximum));
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

// NTSC-U/C: 0x001aba30, PAL: 0x001b1798
Catcher::Catcher(PhraseMgr *pPhraseMgr,
                 Quantizer *pQuantizer,
                 const TrackData *pTrackData,
                 Sch::TickClock *pClock,
                 int nSeekerBarCount,
                 Sch::Tick catchWindow)
    : mQuantizer(pQuantizer), mPhraseMgr(pPhraseMgr), mTrackData(pTrackData),
      mPlayer(&g_nullPlayer), mClock(pClock), mSeekerBarCount(nSeekerBarCount),
      mCatchWindow(static_cast<int>(catchWindow.mValue)), mEnabled(kEnabledInitially),
      mLastCaughtPosition(kNoPosition), mLastMissedPosition(kNoPosition),
      mTrack(pTrackData->mIndex), mCaughtGems(0), mMissedGems(0), mMuffedGems(0), mLastEndedBar(0),
      mPhraseRunBars(0), mLastMuffedBar(kNoBar), mSeekerEnabled(0), mRemotePlayer(&g_nullPlayer),
      mRemoteSuccess(0), mRemotePosition(0) {
    mPostGemCommand.mValue = kUnallocatedCommand;
    mGemCommand.mValue = kUnallocatedCommand;
    mTicksPerBar.mTick = pPhraseMgr->mBarTicks;
}

// NTSC-U/C: 0x001abc10, PAL: 0x001b1978
Catcher::~Catcher() {
    Stop();
}

// NTSC-U/C: 0x001abe50, PAL: 0x001b1bb8
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

// NTSC-U/C: 0x001abfd8, PAL: 0x001b1d40
void Catcher::CatchGem(int nTick, int nGem) {
    // The tick is stored without the finiteness check.
    mLastCaughtPosition.mTick = nTick;
    ++mCaughtGems;

    const int nBar = nTick / mTicksPerBar.mTick;
    const Mid::MBT barStart = MakePosition(mTicksPerBar.mTick * nBar);
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
    Mid::MBT position;
    position.mTick = nTick;
    GemMsg gemMsg(position, mTrack, nGem, mPlayer);
    Send(&gemMsg);

    mPlayer->CountCaughtGem(); // Yes, the binary discards this call's result.

    const int nNextBar = FindNextGemTick(nTick) / mTicksPerBar.mTick;
    if (nNextBar != nBar) {
        PostCaughtBarMsg(nBar, nNextBar);
    }
}

// NTSC-U/C: 0x001abcf8, PAL: 0x001b1a60
int Catcher::SnapToNearestGem(int nTick) {
    Mid::MBT before(kMBTMinimum);
    Mid::MBT after(kMBTMaximum);
    int nBeforeGem;
    int nAfterGem;
    mTrackData->FindGemAtOrBefore(nTick, &before.mTick, &nBeforeGem);
    mTrackData->FindGemAtOrAfter(nTick, &after.mTick, &nAfterGem);

    const Mid::MBT toBefore = MakePosition(nTick - before.mTick);
    const Mid::MBT toAfter = MakePosition(after.mTick - nTick);
    if (toBefore.mTick < toAfter.mTick) {
        if (!(mCatchWindow < toBefore.mTick)) {
            return before.mTick;
        }
    } else if (!(mCatchWindow < toAfter.mTick)) {
        return after.mTick;
    }
    return nTick;
}

// NTSC-U/C: 0x001ac370, PAL: 0x001b20d8
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

// NTSC-U/C: 0x001ac550, PAL: 0x001b22b8
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
    mLastCaughtPosition = Mid::MBT(kNoPosition);
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

// NTSC-U/C: 0x001ac688, PAL: 0x001b23f0
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
    if (nBar == nNow / Mid::MBT(kBarTicks).mTick) {
        const Mid::MBT offset(nNow % Mid::MBT(kBarTicks).mTick);
        mPhraseMgr->ReplayBar(nBar, offset.mTick);
    }
}

// NTSC-U/C: 0x001ac7e8, PAL: 0x001b2550
void Catcher::PostCaughtBarMsg(int nBar, int nNextBar) {
    if ((mMissedGems + mMuffedGems) != 0) {
        return;
    }

    ++mPhraseRunBars;
    CaughtBarMsg msg(mPlayer, nBar);
    mPlayer->Handle(&msg);
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

// NTSC-U/C: 0x001ac958, PAL: 0x001b26c0
void Catcher::EndBar(int nBar) {
    if (mTrackData->IsStepStart(nBar)) {
        mPhraseRunBars = 0;
    }

    if ((mMissedGems + mMuffedGems) > 0 || mCaughtGems == 0) {
        mPhraseRunBars = 0;
        if (mCaughtGems > 0) {
            // Beat-sized, not bar-sized. That is what the binary computes.
            const Mid::MBT position = MakePosition(nBar * Mid::MBT(kBeatTicks).mTick);
            PostPhraseMuffedMsg(nBar - 1, position);
        }
    }

    mLastEndedBar = nBar;
    mMuffedGems = 0;
    mCaughtGems = 0;
    mMissedGems = 0;
}

// NTSC-U/C: 0x001aca48, PAL: 0x001b27b0
int Catcher::FindNextGemTick(int nTick) {
    Mid::MBT next;
    int nGem;
    const Mid::MBT start = MakePosition(nTick + Mid::MBT(1).mTick);
    if (mTrackData->FindGemAtOrAfter(start.mTick, &next.mTick, &nGem)) {
        return next.mTick;
    }

    const Mid::MBT span =
        MakePosition((mTrackData->mGemSearchBars - 1) * Mid::MBT(kBarTicks).mTick);
    next = MakePosition(nTick + span.mTick);
    return next.mTick;
}

// NTSC-U/C: 0x001acba0, PAL: 0x001b2908
void Catcher::SchedulePostGemCommand(int nTick) {
    const int nGemTick = FindNextGemTick(nTick);
    const int nDelay = PostGemDelay(nGemTick);
    const Mid::MBT when = MakePosition(nDelay + Mid::MBT(1).mTick);

    PostGemCmd *pCommand = new PostGemCmd(this, nGemTick);
    mClock->PostAtSongTick(pCommand, when.mTick, mPostGemCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// NTSC-U/C: 0x001acca0, PAL: 0x001b2a08
void Catcher::ScheduleGemCommand(int nTick) {
    const int nGemTick = FindNextGemTick(nTick);

    GemCmd *pCommand = new GemCmd(this, nGemTick);
    mClock->PostAtSongTick(pCommand, nGemTick, mGemCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// NTSC-U/C: 0x001acd30, PAL: 0x001b2a98
int Catcher::PostGemDelay(int nTick) {
    const int nNext = FindNextGemTick(nTick);
    Mid::MBT delay(MakePosition(nTick + nNext).mTick / 2);
    if (MakePosition(nTick + mCatchWindow).mTick < delay.mTick) {
        delay = MakePosition(nTick + mCatchWindow);
    }
    return delay.mTick;
}

// NTSC-U/C: 0x001ace78, PAL: 0x001b2be0
void Catcher::SimulateRemoteGem(int nTick) {
    if (mRemotePlayer != &g_nullPlayer && mRemotePlayer->GetInputSlot() == kNoPlayerSlot) {
        const Mid::MBT window = MakePosition(mRemotePosition.mTick + Mid::MBT(kBarTicks).mTick);
        if (!(window.mTick < nTick) &&
            static_cast<float>(std::rand() % kRandomScale) < mRemoteSuccess * kRandomScale) {
            Mid::MBT gemTick;
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
            Mid::MBT position;
            position.mTick = nTick;
            GemMsg gemMsg(position, mTrack, nGem, mRemotePlayer);
            Send(&gemMsg);
        }
    }
    ScheduleGemCommand(nTick);
}

// NTSC-U/C: 0x001ad0e8, PAL: 0x001b2e50
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

        const Mid::MBT start = MakePosition(Mid::MBT(kBarTicks).mTick * nFirstBar);
        const Mid::MBT beforeStart = MakePosition(start.mTick - Mid::MBT(1).mTick);
        const int nGemTick = FindNextGemTick(beforeStart.mTick);
        const int nGemBar = nGemTick / Mid::MBT(kBarTicks).mTick;

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

// NTSC-U/C: 0x001ad3b0, PAL: 0x001b3118
void Catcher::PostPhraseMuffedMsg(int nBar, Mid::MBT position) {
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

// NTSC-U/C: 0x001ad4e0, PAL: 0x001b3248
void Catcher::PostSeekerMsg() {
    SeekerMsg msg(mPlayer);
    Send(&msg);
    mSeekerEnabled = 0;
}

// NTSC-U/C: 0x001ad560, PAL: 0x001b32c8
void Catcher::PostSeekerRangeMsg(int nFirstBar, int nBarCount) {
    SeekerMsg msg(mPlayer, nFirstBar, nBarCount, mTrack, kSeekerOn, Mid::MBT(0));
    Send(&msg);
    mSeekerEndBar = nFirstBar + nBarCount;
    mSeekerEnabled = kSeekerOn;
    mSeekerFirstBar = nFirstBar;
}

// NTSC-U/C: 0x001adb78, PAL: 0x001b38e0
void Catcher::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nPitchRiffMsgType) {
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
    } else if (nType == g_nInvalidateSeekerMsgType) {
        OnInvalidateSeeker(static_cast<InvalidateSeekerMsg *>(pMsg));
    }
}

// NTSC-U/C: 0x001b1488, PAL: 0x001b7248
int Catcher::IsBarFree(int nBar) {
    if (nBar < 0) {
        return 0;
    }
    if (mTrackData->QueryBar(nBar) == 0) {
        return 0;
    }
    return mPhraseMgr->GetPhraseOwner(nBar)->IsNull() != 0;
}

// NTSC-U/C: 0x001b1578, PAL: 0x001b7338
void Catcher::OnInvalidateSeeker(InvalidateSeekerMsg *pMsg) {
    if (pMsg->mTrack == mTrack) {
        UpdateSeeker(pMsg->mBar);
    }
}

// NTSC-U/C: 0x001b15a8, PAL: 0x001b7368
void Catcher::Start() {
    SchedulePostGemCommand(Mid::MBT(kBeforeSongStart).mTick);
    if (Application::shared()->GetGameMode() == kGameModeNet) {
        ScheduleGemCommand(Mid::MBT(kBeforeSongStart).mTick);
    }
}

// NTSC-U/C: 0x001b1610, PAL: 0x001b73d0
void Catcher::Stop() {
    mClock->Withdraw(mPostGemCommand);
    if (Application::shared()->GetGameMode() == kGameModeNet) {
        mClock->Withdraw(mGemCommand);
    }
}

// NTSC-U/C: 0x001b1828, PAL: 0x001b75e8
void Catcher::ProcessGemCommand(int nTick) {
    if (mLastCaughtPosition.mTick != nTick) {
        mPhraseRunBars = 0;
        ++mMuffedGems;

        // The position is passed without the finiteness check.
        Mid::MBT position;
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

// NTSC-U/C: 0x001b1918, PAL: 0x001b76d8
void Catcher::SetPhraseOwners(int nFirstBar, int nEndBar, Player *pPlayer) {
    (void)pPlayer->IsNull(); // Yes, the binary discards this call's result.
    for (int nBar = nFirstBar; nBar < nEndBar; ++nBar) {
        mPhraseMgr->SetPhraseOwner(pPlayer, nBar);
    }
}

// NTSC-U/C: 0x001b19a0, PAL: 0x001b7760
int Catcher::IsPhraseRunEmpty() {
    return mPhraseRunBars == 0;
}
