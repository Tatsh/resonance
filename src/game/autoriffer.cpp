#include "game/autoriffer.h"

#include <algorithm>
#include <cstring>
#include <iostream>

#include "app/playsound.h"
#include "game/nullplayer.h"
#include "mid/mbt.h"
#include "msg/allnotesoffmsg.h"
#include "msg/axebuttonmsg.h"
#include "msg/gameovermsg.h"
#include "msg/multimusemsg.h"
#include "sch/command.h"

namespace {

// The handle value of a command the clock has not queued yet.
constexpr int kUnallocatedCommand = -2;

// One bar at 480 ticks per quarter note.
constexpr int kBarTicks = 1920;

// The difficulty levels mLevelHeld records.
constexpr int kLevelCount = 4;

// The flag a held difficulty level carries, and the button states AxeButtonMsg reports.
constexpr int kHeld = 1;
constexpr int kNotHeld = 0;
constexpr int kPressed = 1;
constexpr int kReleased = 0;

// The clamp the inline Mid::MBT arithmetic applies to a computed position.
inline int ClampPosition(int nTick) {
    return std::min(std::max(nTick, kMBTMinimum), kMBTMaximum);
}

/**
 * Scheduler command that runs AutoRiffer::OnCommand() at one song position.
 *
 * `Q233_GLOBAL_$N$GsAutoRiffer.cppdKuhgb3Cmd` in the RTTI, with Sch::Command as its one base and
 * its vtable at `0x007dd3a8`. AutoRiffer::PlayRiff() expands the constructor into its 0x14-byte
 * allocation.
 *
 * The destructor at `0x0019a7c0` is implicitly declared.
 */
class Cmd : public Sch::Command {
public:
    Cmd(AutoRiffer *pOwner, int nTick) : mOwner(pOwner), mTick(nTick) {
    }

    // 0x0019a838
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x0019a848
    virtual void Execute() {
        mOwner->OnCommand(mTick);
    }

    // 0x0019a868
    virtual void Print(std::ostream &stream) {
        stream << "{AutoRiffer}";
    }

    // The word at 0x006815e0, which the image initialises to zero.
    static int sCmdID;

private:
    AutoRiffer *mOwner; // +0x0c
    int mTick;          // +0x10
};

int Cmd::sCmdID;

} // namespace

// 0x00199040
AutoRiffer::AutoRiffer(Sch::TickClock *pClock, Quantizer *pQuantizer, const TrackData *pTrackData)
    : mTrack(pTrackData->mUnknown04), mQuantizer(pQuantizer), mTrackData(pTrackData),
      mCurrentRiff(nullptr), mClock(pClock), mSynth(nullptr), mPhraseMaker(nullptr),
      mPlayer(&g_nullPlayer) {
    mCommand.mValue = kUnallocatedCommand;
    std::memset(mLevelHeld, 0, sizeof(mLevelHeld));
}

// 0x00199160
void AutoRiffer::OnPitchRiff(PitchRiffMsg *pMsg) {
    if (pMsg->mUnknown10 != mTrack) {
        return;
    }
    if (pMsg->mUnknown08 != mPlayer) {
        return;
    }

    const int nTick = pMsg->mUnknown0c.mTick;
    const unsigned int nQuantized = mQuantizer->Quantize(nTick);
    if (mPhraseMaker != nullptr &&
        mPhraseMaker->IsBarPlayable(nQuantized / Mid::MBT(kBarTicks).mTick) != 1) {
        PlaySoundByName("SND_INACTIVE");
        return;
    }

    const int nLevel = pMsg->mUnknown04;
    Riff *pRiff = mTrackData->GetRiff(nQuantized, nLevel);
    if (pRiff == nullptr) {
        return;
    }
    if (mCurrentRiff != nullptr) {
        mClock->Withdraw(mCommand);
    }
    mCurrentRiff = pRiff;
    mLevelHeld[nLevel] = kHeld;
    PlayRiff(nTick);

    AxeButtonMsg press(kPressed, 0, pMsg->mUnknown08);
    mSource.Send(&press);
}

// 0x001992e0
void AutoRiffer::OnStopRiff(StopRiffMsg *pMsg) {
    if (pMsg->mUnknown10 != mTrack) {
        return;
    }
    if (pMsg->mPlayer != mPlayer) {
        return;
    }
    if (mCurrentRiff == nullptr) {
        return;
    }

    const int nTick = pMsg->mPosition.mTick;
    const int nQuantized = static_cast<int>(mQuantizer->Quantize(nTick));
    mLevelHeld[pMsg->mUnknown04] = kNotHeld;
    for (int nLevel = 0; nLevel < kLevelCount; ++nLevel) {
        if (mLevelHeld[nLevel] != kNotHeld) {
            mCurrentRiff = mTrackData->GetRiff(nQuantized, nLevel);
            mClock->Withdraw(mCommand);
            PlayRiff(nTick);
            return;
        }
    }

    AllNotesOffMsg notesOff(nTick);
    mSource.Send(&notesOff);
    mClock->Withdraw(mCommand);
    mCurrentRiff = nullptr;

    AxeButtonMsg release(kReleased, 0, pMsg->mPlayer);
    mSource.Send(&release);
}

// 0x00199480
void AutoRiffer::OnErase(EraseMsg *pMsg) {
    if (pMsg->mUnknown0c != mTrack) {
        return;
    }
    if (pMsg->mUnknown04 != mPlayer) {
        return;
    }
    if (!mPhraseMaker->IsBarPlayable(pMsg->mUnknown08.mTick / Mid::MBT(kBarTicks).mTick)) {
        return;
    }

    StopRiff(pMsg->mUnknown08.mTick);
    AllNotesOffMsg notesOff;
    mSynth->Handle(&notesOff);
    mPhraseMaker->Erase(pMsg->mUnknown04, pMsg->mUnknown08.mTick, pMsg->mUnknown10);
}

// 0x00199590
void AutoRiffer::StopRiff(int nTick) {
    if (mCurrentRiff == nullptr) {
        return;
    }

    std::memset(mLevelHeld, 0, sizeof(mLevelHeld));
    AllNotesOffMsg notesOff(nTick);
    mSource.Send(&notesOff);
    mClock->Withdraw(mCommand);
    mCurrentRiff = nullptr;

    AxeButtonMsg release(kReleased, 0, mPlayer);
    mSource.Send(&release);
}

// 0x00199688
void AutoRiffer::OnCommand(int nTick) {
    if (mPhraseMaker->IsBarPlayable(nTick / Mid::MBT(kBarTicks).mTick) == 1) {
        PlayRiff(nTick);
        return;
    }
    AxeButtonMsg release(kReleased, 0, mPlayer);
    mSource.Send(&release);
}

// 0x00199758
void AutoRiffer::PlayRiff(int nTick) {
    AllNotesOffMsg notesOff;
    mSynth->Handle(&notesOff);

    MultiMuseMsg riffMsg(mCurrentRiff);
    mSource.Send(&riffMsg);

    (void)Mid::MBT(0); // Yes, the binary discards this position.
    int nEnd = static_cast<int>(Quantizer::Round(nTick, mCurrentRiff->mLength.mTick));
    if (!(nTick < nEnd)) {
        // The sum is clamped without the finiteness check.
        nEnd = ClampPosition(nEnd + mCurrentRiff->mLength.mTick);
    }

    Cmd *pCommand = new Cmd(this, nEnd);
    mClock->PostAtSongTick(pCommand, nEnd, mCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// 0x00199910
void AutoRiffer::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nPitchRiffMsgType) {
        OnPitchRiff(static_cast<PitchRiffMsg *>(pMsg));
        return;
    }
    if (nType == g_nEraseMsgType) {
        OnErase(static_cast<EraseMsg *>(pMsg));
        return;
    }
    if (nType == g_nStopRiffMsgType) {
        OnStopRiff(static_cast<StopRiffMsg *>(pMsg));
        return;
    }
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        TrackSelectMsg *pSelect = static_cast<TrackSelectMsg *>(pMsg);
        if (pSelect->mUnknown04 != mTrack || pSelect->mUnknown08 != 0) {
            return;
        }
        Player *pPlayer = pSelect->mUnknown10;
        if (pPlayer != mPlayer || pPlayer->IsNull()) {
            StopRiff(pSelect->mPosition.mTick);
        }
        mPlayer = pPlayer;
        return;
    }
    if (nType == g_nGameOverMsgType) {
        StopRiff(Mid::MBT(0).mTick);
    }
}

// 0x0019a3d0
AutoRiffer::~AutoRiffer() {
}

// 0x0019a508
void AutoRiffer::AddSink(MsgSink *pSink) {
    mSource.AddSink(pSink);
}

// 0x0019a898
void AutoRiffer::OnTrackSelect(TrackSelectMsg *pMsg) {
    if (pMsg->mUnknown04 != mTrack || pMsg->mUnknown08 != 0) {
        return;
    }
    Player *pPlayer = pMsg->mUnknown10;
    if (pPlayer != mPlayer || pPlayer->IsNull()) {
        StopRiff(pMsg->mPosition.mTick);
    }
    mPlayer = pPlayer;
}

// 0x0019a920
void AutoRiffer::OnGameOver() {
    StopRiff(Mid::MBT(0).mTick);
}
