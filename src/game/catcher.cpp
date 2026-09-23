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

// The seeker states PostSeekerRangeMsg() records and sends.
constexpr int kSeekerOn = 1;

// OnAutoCatch() plays its bar through slot 9 with both flags set, and marks the message handled.
// PostCaughtBarMsg() clears the third.
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

// The four player slots Player::Slot2() reports, each with its own miss sound.
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

// 0x001abe50
void Catcher::Slot7(int nTick, int nGem) {
    switch (mPlayer->Slot2()) {
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
    if (nBar == mUnknown5c || mUnknown60 > 0) {
        // The tick is stored without the finiteness check.
        mUnknown48.mTick = nTick;
        mUnknown60 = 0;
        ++mUnknown54;
        PostPhraseMuffedMsg(nBar, mUnknown48);
        UpdateSeeker(nBar);
    }
}

// 0x001abfd8
void Catcher::Slot8(int nTick, int nGem) {
    // The tick is stored without the finiteness check.
    mUnknown44.mTick = nTick;
    ++mUnknown50;

    const int nBar = nTick / mTicksPerBar.mTick;
    const Mid::MBT barStart = MakePosition(mTicksPerBar.mTick * nBar);
    if (mUnknown48.mTick < barStart.mTick) {
        mUnknown54 = 0;
    }

    MultiMuseMsg riffMsg(mTrackData->GetRiff(nTick, nGem));
    Send(&riffMsg);

    int nPoints = 0;
    int nCaught = 0;
    int nTotal = 0;
    if (mUnknown54 == 0 && mUnknown58 == 0) {
        const int nEndBar = mSeekerEndBar;
        nCaught = mUnknown50;
        if (!(mTrackData->FollowingStepBar(nBar) < nEndBar)) {
            for (int nPhraseBar = nBar - mUnknown60; nPhraseBar < nEndBar; ++nPhraseBar) {
                const int nGems = static_cast<int>(mTrackData->GetGems(nPhraseBar)->size());
                nPoints += mTrackData->GetPoints(nPhraseBar);
                nTotal += nGems;
                nCaught += (nPhraseBar < nBar) * nGems;
            }
            if (nCaught == 1) {
                BeginPhraseCatchMsg beginMsg(mPlayer, nPoints, mPlayer->Slot16(nBar));
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

    mPlayer->Slot14(); // Yes, the binary discards this call's result.

    const int nNextBar = FindNextGemTick(nTick) / mTicksPerBar.mTick;
    if (nNextBar != nBar) {
        PostCaughtBarMsg(nBar, nNextBar);
    }
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

// 0x001ac370
void Catcher::PostCatchMsg(PitchRiffMsg *pMsg) {
    if (pMsg->mUnknown10 != mTrack) {
        return;
    }
    if (pMsg->mUnknown08 != mPlayer) {
        pMsg->mUnknown08->Slot5(); // Yes, the binary discards this call's result.
        PlaySoundByName("SND_INACTIVE");
        return;
    }

    const int nTick = pMsg->mUnknown0c.mTick;
    const int nSnapped = SnapToNearestGem(nTick);
    const int nGem = pMsg->mUnknown04;
    if (!IsBarFree(nSnapped / mTicksPerBar.mTick)) {
        PlaySoundByName("SND_INACTIVE");
        CatchMsg msg(nTick, mTrack, nGem, kCatchMiss, mPlayer, kNoProgress, kNoProgress);
        Send(&msg);
        return;
    }

    const int nGemAt = mTrackData->GetGemAt(nSnapped);
    if (nGemAt != kNoGem && nGemAt == nGem && mUnknown44.mTick != nSnapped) {
        Slot8(nSnapped, nGem);
    } else {
        Slot7(nTick, nGem);
    }
}

// 0x001ac550
void Catcher::OnTrackSelect(TrackSelectMsg *pMsg) {
    if (pMsg->mUnknown04 != mTrack) {
        return;
    }
    if (pMsg->mUnknown08 != 0) {
        return;
    }

    const int nBar = pMsg->mPosition.mTick / mTicksPerBar.mTick;
    Player *pNewPlayer = pMsg->mUnknown10;
    if (!mPlayer->IsNull() && mPlayer != pNewPlayer && mPlayer->Slot2() != kNoPlayerSlot) {
        PostSeekerMsg();
    }
    mPlayer = pNewPlayer;
    mUnknown44 = Mid::MBT(kNoPosition);
    mUnknown60 = 0;
    mUnknown58 += mUnknown50;

    if (!mPlayer->IsNull()) {
        mUnknown50 = 0;
        UpdateSeeker(nBar);
        return;
    }
    if (mUnknown50 > 0 || nBar < mUnknown5c) {
        mUnknown64 = nBar;
    }
}

// 0x001ac688
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
    Slot9(nBar, kAutoCatchFlag, kAutoCatchFlag);
    mPlayer = pSavedPlayer;
    pMsg->mUnknown04 = kMessageHandled;
    UpdateSeeker(nBar);

    const int nNow = Application::shared()->GetSongClock()->SongTick();
    if (nBar == nNow / Mid::MBT(kBarTicks).mTick) {
        const Mid::MBT offset(nNow % Mid::MBT(kBarTicks).mTick);
        mPhraseMgr->ReplayBar(nBar, offset.mTick);
    }
}

// 0x001ac7e8
void Catcher::PostCaughtBarMsg(int nBar, int nNextBar) {
    if ((mUnknown54 + mUnknown58) != 0) {
        return;
    }

    ++mUnknown60;
    CaughtBarMsg msg(mPlayer, nBar);
    mPlayer->Handle(&msg);
    Slot10(nBar);

    int nLastBar = nBar;
    if ((nBar + 1) < nNextBar) {
        nLastBar = std::min(nNextBar - 1, mTrackData->FollowingStepBar(nBar) - 1);
        mUnknown60 += nLastBar - nBar;
        if (!(mUnknown60 < mSeekerBarCount)) {
            nLastBar -= mUnknown60 - mSeekerBarCount;
            mUnknown60 = mSeekerBarCount;
        }
    }

    if (mSeekerEnabled != 0 && (nLastBar + 1) == mSeekerEndBar) {
        Slot9(nLastBar, mUnknown60, kNoAutoCatchFlag);
        mUnknown60 = 0;
        UpdateSeeker(nLastBar);
    }
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
            PostPhraseMuffedMsg(nBar - 1, position);
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

// 0x001ace78
void Catcher::SimulateRemoteGem(int nTick) {
    if (mRemotePlayer != &g_nullPlayer && mRemotePlayer->Slot2() == kNoPlayerSlot) {
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

// 0x001ad3b0
void Catcher::PostPhraseMuffedMsg(int nBar, Mid::MBT position) {
    if (nBar == mUnknown64) {
        return;
    }
    mUnknown64 = nBar;
    if (mPlayer->IsNull()) {
        return;
    }

    int nTried = 0;
    if (IsBarFree(nBar)) {
        nTried = (mUnknown54 + mUnknown50) > 0;
    }
    PhraseMuffedMsg msg(mTrack, mPlayer, position, nTried);
    Send(&msg);
}

// 0x001ad4e0
void Catcher::PostSeekerMsg() {
    SeekerMsg msg(mPlayer);
    Send(&msg);
    mSeekerEnabled = 0;
}

// 0x001ad560
void Catcher::PostSeekerRangeMsg(int nFirstBar, int nBarCount) {
    SeekerMsg msg(mPlayer, nFirstBar, nBarCount, mTrack, kSeekerOn, Mid::MBT(0));
    Send(&msg);
    mSeekerEndBar = nFirstBar + nBarCount;
    mSeekerEnabled = kSeekerOn;
    mSeekerFirstBar = nFirstBar;
}

// 0x001adb78
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

        // The position is passed without the finiteness check.
        Mid::MBT position;
        position.mTick = nTick;
        PostPhraseMuffedMsg(nTick / mTicksPerBar.mTick, position);
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
