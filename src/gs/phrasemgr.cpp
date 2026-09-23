#include "gs/phrasemgr.h"

#include <algorithm>
#include <iostream>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/jampowerbarmgr.h"
#include "game/multipowerbarmgr.h"
#include "game/nullplayer.h"
#include "game/phrase.h"
#include "game/phrasedatabase.h"
#include "game/phraseplayer.h"
#include "game/playmap.h"
#include "game/solopowerbarmgr.h"
#include "game/trackdata.h"
#include "sch/command.h"
#include "sch/tickclock.h"
#include "script/configquery.h"

namespace {

// The handle value of a command the clock has not queued yet.
constexpr int kUnallocatedCommand = -2;

// A display-mode configuration flag. When it is set, every track gets a JamPowerbarMgr.
constexpr int kDisplayModeQuery = 0x3a1;

// The clamp the inline Mid::MBT arithmetic applies to a computed position.
inline int ClampPosition(int nTick) {
    return std::min(std::max(nTick, kMBTMinimum), kMBTMaximum);
}

/**
 * Scheduler command that runs PhraseMgr::OnCommand() at the start of a bar.
 *
 * `Q232_GLOBAL_$N$GsPhraseMgr.cppvNNjgb3Cmd` in the RTTI (descriptor `0x008f0960`), with
 * Sch::Command as its one base and its vtable at `0x007e2740`. PhraseMgr::OnCommand() expands the
 * constructor into its 0x14-byte allocation.
 *
 * The destructor at `0x001bfe60` is implicitly declared.
 */
class Cmd : public Sch::Command {
public:
    Cmd(PhraseMgr *pOwner, int nBar) : mOwner(pOwner), mBar(nBar) {
    }

    // 0x001bfed8
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x001bfee8
    virtual void Execute() {
        mOwner->OnCommand(mBar);
    }

    // 0x001bff08
    virtual void Print(std::ostream &stream) {
        stream << "{PhraseMgr}";
    }

    // The word at 0x0068a058, which the image initialises to zero.
    static int sCmdID;

private:
    PhraseMgr *mOwner; // +0x0c
    int mBar;          // +0x10
};

int Cmd::sCmdID;

/**
 * Scheduler command that runs PhraseMgr::OnExportCommand() shortly after the start of a bar.
 *
 * `Q232_GLOBAL_$N$GsPhraseMgr.cppvNNjgb9ExportCmd` in the RTTI (descriptor `0x009021f0`), with
 * Sch::Command as its one base and its vtable at `0x007e26f8`. PhraseMgr::OnExportCommand()
 * expands the constructor into its 0x14-byte allocation.
 *
 * The destructor at `0x001bff38` is implicitly declared.
 */
class ExportCmd : public Sch::Command {
public:
    ExportCmd(PhraseMgr *pOwner, int nBar) : mOwner(pOwner), mBar(nBar) {
    }

    // 0x001bffb0
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x001bffc0
    virtual void Execute() {
        mOwner->OnExportCommand(mBar);
    }

    // 0x001bffe0
    virtual void Print(std::ostream &stream) {
        stream << "{PhraseMgr::Export}";
    }

    // The word at 0x0068a064, which the image initialises to zero.
    static int sCmdID;

private:
    PhraseMgr *mOwner; // +0x0c
    int mBar;          // +0x10
};

int ExportCmd::sCmdID;

} // namespace

// 0x001ba0d0
PhraseMgr::PhraseMgr(
    Sch::TickClock *pClock, int nBarTicks, PlayMap *pMap, int nConfig, const TrackData *pTrackData)
    : mPhrasePlayer(nullptr), mUnknown1c(0), mTrackData(pTrackData), mMap(pMap),
      mPowerbarMgr(nullptr), mUnknown30(pTrackData->mUnknown04), mBarTicks(nBarTicks),
      mConfig(nConfig), mWindowStart(0), mWindowEnd(0), mRefreshing(0), mExportLead(0),
      mClock(pClock), mTrackKind(pTrackData->mKind) {
    mExportCommand.mValue = kUnallocatedCommand;
    mCommand.mValue = kUnallocatedCommand;
    mPlayMode = Application::shared()->GetPlayMode();
    mDatabase = new PhraseDatabase(pMap);
    CreatePowerbarMgr();
}

// 0x001ba2b8
PhraseMgr::~PhraseMgr() {
    WithdrawCommands();
    delete mDatabase;
    delete mPowerbarMgr;
}

// 0x001ba3d8
void PhraseMgr::CreatePowerbarMgr() {
    delete mPowerbarMgr;
    mPowerbarMgr = nullptr;

    const int nGameMode = Application::shared()->GetGameMode();
    if (mPlayMode == kPlayModeGame &&
        (mTrackKind == kTrackModeCatch || mTrackKind == kTrackModeRiff) &&
        !QueryConfigFlag(kDisplayModeQuery)) {
        if (nGameMode == kGameModeSolo) {
            mPowerbarMgr = new SoloPowerbarMgr(mMap, mDatabase, mTrackData, mUnknown30);
        } else {
            mPowerbarMgr = new MultiPowerbarMgr(mMap, mDatabase, mTrackData, mUnknown30);
        }
        return;
    }
    mPowerbarMgr = new JamPowerbarMgr;
}

// 0x001bb6b8
void PhraseMgr::OnCommand(int nBar) {
    mPhrasePlayer->PlayBar(nBar);

    const int nNextBar = nBar + 1;
    Cmd *pCommand = new Cmd(this, nNextBar);
    const Mid::MBT when(ClampPosition(mBarTicks * nNextBar));
    mClock->PostAtSongTick(pCommand, when.mTick, mCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// 0x001bb8a0
void PhraseMgr::OnExportCommand(int nBar) {
    mRefreshing = 1;
    mWindowStart = nBar - 1;
    mWindowEnd = nBar + mConfig + 1;
    RefreshBar(nBar + mConfig, 0);
    mRefreshing = 0;

    const int nNextBar = nBar + 1;
    ExportCmd *pCommand = new ExportCmd(this, nNextBar);
    const Mid::MBT start(ClampPosition(mBarTicks * nNextBar));
    const Mid::MBT when(ClampPosition(start.mTick + mExportLead.mTick));
    mClock->PostAtSongTick(pCommand, when.mTick, mExportCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// 0x001bf6c0
int PhraseMgr::TickToBar(int nTick) {
    const int nBar = nTick / mBarTicks;
    (void)Mid::MBT(ClampPosition(mBarTicks * nBar)); // Yes, the binary discards this position.
    return nBar;
}

// 0x001bf738
int PhraseMgr::BarToTick(int nBar) {
    return Mid::MBT(ClampPosition(mBarTicks * nBar)).mTick;
}

// 0x001c01c8
Phrase *PhraseMgr::GetPhraseAt(int nTick) {
    return mDatabase->GetPhraseAt(nTick);
}

// 0x001c01e8
int PhraseMgr::GetPowerbar(int nBar) {
    return mPowerbarMgr->GetPowerbar(mMap->Slot5(nBar));
}

// 0x001c0248
long *PhraseMgr::GetStepValue(int nTick) {
    return mDatabase->GetStepValue(nTick);
}

// 0x001c0268
Player *PhraseMgr::GetPhraseOwner(int nBar) {
    Phrase *pPhrase = mDatabase->GetPhraseAt(nBar);
    if (pPhrase == nullptr) {
        return &g_nullPlayer;
    }
    return pPhrase->mPlayer;
}

// 0x001c0298
void PhraseMgr::SetPhraseByte(int nBar, char cValue) {
    const int nFirst = mMap->Slot5(nBar);
    int nIndex = nFirst;
    do {
        mDatabase->SetPhraseByte(nIndex, cValue);
        nIndex = mMap->Slot7(nIndex, mUnknown30);
    } while (nIndex != nFirst);
}

// 0x001c0338
unsigned char PhraseMgr::GetPhraseByte(int nBar) {
    return mDatabase->GetPhraseByte(mMap->Slot5(nBar));
}

// 0x001c0380
void PhraseMgr::ResetOwners(Player *pPlayer) {
    mDatabase->SetOwners(pPlayer);
    for (int nBar = mWindowStart; nBar < mWindowEnd; ++nBar) {
        RefreshBar(nBar, 1);
    }
}

// 0x001c03e0
void PhraseMgr::RefreshAllBars() {
    mRefreshing = 1;
    mWindowStart = 0;
    mWindowEnd = mConfig + 1;
    for (int nBar = 0; nBar < mWindowEnd; ++nBar) {
        RefreshBar(nBar, 0);
    }
    mRefreshing = 0;
}

// 0x001c0450
void PhraseMgr::WithdrawCommands() {
    mClock->Withdraw(mCommand);
    mClock->Withdraw(mExportCommand);
}
