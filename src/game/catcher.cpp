#include "game/catcher.h"

#include <algorithm>
#include <iostream>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/nullplayer.h"
#include "mid/mbt.h"
#include "sch/command.h"

namespace {

// Slot 4 searches for the first gem from the position before the song starts.
constexpr int kBeforeSongStart = -1;

// The handle value of a command the clock has not queued yet.
constexpr int kUnallocatedCommand = -2;

// The position the constructor gives mUnknown44 and mUnknown48.
constexpr int kNoPosition = -1;

// The bar the constructor gives mUnknown64.
constexpr int kNoBar = -1;

// The value the constructor gives mUnknown40.
constexpr int kUnknown40Initial = 1;

// Player::Slot2() reports this for a player without a slot.
constexpr int kNoPlayerSlot = -1;

// One bar and one beat at 480 ticks per quarter note.
constexpr int kBarTicks = 1920;
constexpr int kBeatTicks = 480;

// The bars UpdateSeeker() scans for a free bar.
constexpr int kSeekerScanBars = 32;

// A computed position, clamped to the finite range as the inline Mid::MBT arithmetic does.
inline Mid::MBT MakePosition(int nTick) {
    return Mid::MBT(std::min(std::max(nTick, kMBTMinimum), kMBTMaximum));
}

/**
 * Scheduler command that runs Catcher::ProcessGemCommand() after a gem.
 *
 * `Q285_GLOBAL_$N$__7CatcherP9PhraseMgrP9QuantizerPC9TrackDataPQ23Sch9TickClockiGQ23Sch4Tick10PostGemCmd`
 * in the RTTI, with Sch::Command as its one base and its vtable at `0x007e0998`.
 * Catcher::SchedulePostGemCommand() expands the constructor into its 0x14-byte allocation.
 *
 * The destructor at `0x001b1678` is implicitly declared.
 */
class PostGemCmd : public Sch::Command {
public:
    PostGemCmd(Catcher *pOwner, int nTick) : mOwner(pOwner), mTick(nTick) {
    }

    // 0x001b16f0
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x001b1700
    virtual void Execute() {
        mOwner->ProcessGemCommand(mTick);
    }

    // 0x001b1720
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
 * `Q285_GLOBAL_$N$__7CatcherP9PhraseMgrP9QuantizerPC9TrackDataPQ23Sch9TickClockiGQ23Sch4Tick6GemCmd`
 * in the RTTI, with Sch::Command as its one base and its vtable at `0x007e0950`.
 * Catcher::ScheduleGemCommand() expands the constructor into its 0x14-byte allocation.
 *
 * The destructor at `0x001b1750` is implicitly declared.
 */
class GemCmd : public Sch::Command {
public:
    GemCmd(Catcher *pOwner, int nTick) : mOwner(pOwner), mTick(nTick) {
    }

    // 0x001b17c8
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x001b17d8
    virtual void Execute() {
        mOwner->SimulateRemoteGem(mTick);
    }

    // 0x001b17f8
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

// 0x001aba30
Catcher::Catcher(PhraseMgr *pPhraseMgr,
                 Quantizer *pQuantizer,
                 const TrackData *pTrackData,
                 Sch::TickClock *pClock,
                 int nSeekerBarCount,
                 Sch::Tick catchWindow)
    : mQuantizer(pQuantizer), mPhraseMgr(pPhraseMgr), mTrackData(pTrackData),
      mPlayer(&g_nullPlayer), mClock(pClock), mSeekerBarCount(nSeekerBarCount),
      mCatchWindow(static_cast<int>(catchWindow.mValue)), mUnknown40(kUnknown40Initial),
      mUnknown44(kNoPosition), mUnknown48(kNoPosition), mTrack(pTrackData->mUnknown04),
      mUnknown50(0), mUnknown54(0), mUnknown58(0), mUnknown5c(0), mUnknown60(0), mUnknown64(kNoBar),
      mSeekerEnabled(0), mRemotePlayer(&g_nullPlayer), mRemoteSuccess(0), mRemotePosition(0) {
    mPostGemCommand.mValue = kUnallocatedCommand;
    mGemCommand.mValue = kUnallocatedCommand;
    mTicksPerBar.mTick = pPhraseMgr->mBarTicks;
}

// 0x001abc10
Catcher::~Catcher() {
    Slot5();
}

// 0x001abcf8
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

// 0x001ac958
void Catcher::EndBar(int nBar) {
    if (mTrackData->IsStepStart(nBar)) {
        mUnknown60 = 0;
    }

    if ((mUnknown54 + mUnknown58) > 0 || mUnknown50 == 0) {
        mUnknown60 = 0;
        if (mUnknown50 > 0) {
            // Beat-sized, not bar-sized. That is what the binary computes.
            const Mid::MBT position = MakePosition(nBar * Mid::MBT(kBeatTicks).mTick);
            PostPhraseMuffedMsg(nBar - 1, position.mTick);
        }
    }

    mUnknown5c = nBar;
    mUnknown58 = 0;
    mUnknown50 = 0;
    mUnknown54 = 0;
}

// 0x001aca48
int Catcher::FindNextGemTick(int nTick) {
    Mid::MBT next;
    int nGem;
    const Mid::MBT start = MakePosition(nTick + Mid::MBT(1).mTick);
    if (mTrackData->FindGemAtOrAfter(start.mTick, &next.mTick, &nGem)) {
        return next.mTick;
    }

    const Mid::MBT span = MakePosition((mTrackData->mUnknown30 - 1) * Mid::MBT(kBarTicks).mTick);
    next = MakePosition(nTick + span.mTick);
    return next.mTick;
}

// 0x001acba0
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

// 0x001acca0
void Catcher::ScheduleGemCommand(int nTick) {
    const int nGemTick = FindNextGemTick(nTick);

    GemCmd *pCommand = new GemCmd(this, nGemTick);
    mClock->PostAtSongTick(pCommand, nGemTick, mGemCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// 0x001acd30
int Catcher::PostGemDelay(int nTick) {
    const int nNext = FindNextGemTick(nTick);
    Mid::MBT delay(MakePosition(nTick + nNext).mTick / 2);
    if (MakePosition(nTick + mCatchWindow).mTick < delay.mTick) {
        delay = MakePosition(nTick + mCatchWindow);
    }
    return delay.mTick;
}

// 0x001ad0e8
void Catcher::UpdateSeeker(int nBar) {
    if (mPlayer->IsNull()) {
        return;
    }
    if (mPlayer->Slot2() == kNoPlayerSlot) {
        return;
    }
    if (mPlayer->Slot5()) {
        PostSeekerMsg();
    }

    const int nStart = (mUnknown64 < nBar) ? nBar : (mUnknown64 + 1);
    int nStepBar = mTrackData->NextStepBar(nStart);
    for (int nScanBar = nStart; nScanBar < (nStart + kSeekerScanBars); ++nScanBar) {
        if (!IsBarFree(nScanBar)) {
            continue;
        }

        const int nFirstBar = nScanBar - mUnknown60;
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

// 0x001b1488
int Catcher::IsBarFree(int nBar) {
    if (nBar < 0) {
        return 0;
    }
    if (mTrackData->QueryBar(nBar) == 0) {
        return 0;
    }
    return mPhraseMgr->GetPhraseOwner(nBar)->IsNull() != 0;
}

// 0x001b1578
void Catcher::OnInvalidateSeeker(InvalidateSeekerMsg *pMsg) {
    if (pMsg->mUnknown08 == mTrack) {
        UpdateSeeker(pMsg->mUnknown04);
    }
}

// 0x001b15a8
void Catcher::Slot4() {
    SchedulePostGemCommand(Mid::MBT(kBeforeSongStart).mTick);
    if (Application::shared()->GetGameMode() == kGameModeNet) {
        ScheduleGemCommand(Mid::MBT(kBeforeSongStart).mTick);
    }
}

// 0x001b1610
void Catcher::Slot5() {
    mClock->Withdraw(mPostGemCommand);
    if (Application::shared()->GetGameMode() == kGameModeNet) {
        mClock->Withdraw(mGemCommand);
    }
}

// 0x001b1828
void Catcher::ProcessGemCommand(int nTick) {
    if (mUnknown44.mTick != nTick) {
        mUnknown60 = 0;
        ++mUnknown58;
        PostPhraseMuffedMsg(nTick / mTicksPerBar.mTick, nTick);
        UpdateSeeker(mUnknown64);
        if (mPlayer != nullptr) {
            mPlayer->Slot15();
        }
    }

    const int nBar = nTick / mTicksPerBar.mTick;
    const int nNextBar = FindNextGemTick(nTick) / mTicksPerBar.mTick;
    if (nBar < nNextBar) {
        EndBar(nNextBar);
    }
    SchedulePostGemCommand(nTick);
}

// 0x001b1918
void Catcher::SetPhraseOwners(int nFirstBar, int nEndBar, Player *pPlayer) {
    (void)pPlayer->IsNull(); // Yes, the binary discards this call's result.
    for (int nBar = nFirstBar; nBar < nEndBar; ++nBar) {
        mPhraseMgr->SetPhraseOwner(pPlayer, nBar);
    }
}

// 0x001b19a0
int Catcher::Slot6() {
    return mUnknown60 == 0;
}
