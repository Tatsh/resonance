#include "gs/phrasemgr.h"

#include <algorithm>
#include <iostream>

#include "app/application.h"
#include "game/axeoldgemmaker.h"
#include "game/gamemanagerimpl.h"
#include "game/grooveworld.h"
#include "game/jampowerbarmgr.h"
#include "game/multipowerbarmgr.h"
#include "game/nullplayer.h"
#include "game/phrase.h"
#include "game/phrasedatabase.h"
#include "game/phraseplayer.h"
#include "game/playmap.h"
#include "game/solopowerbarmgr.h"
#include "game/trackdata.h"
#include "msg/barstatusmsg.h"
#include "msg/caughtphrasepacket.h"
#include "msg/cleargemmsg.h"
#include "msg/cleargemsmsg.h"
#include "msg/durgemmsg.h"
#include "msg/gamebeginmsg.h"
#include "msg/gemmsg.h"
#include "msg/gempacket.h"
#include "msg/phrasemsg.h"
#include "msg/refreshnetmsg.h"
#include "sch/command.h"
#include "sch/tickclock.h"
#include "script/configquery.h"

namespace {

// The handle value of a command the clock has not queued yet.
constexpr int kUnallocatedCommand = -2;

// PostBarStatusMsg() sets every bit of the field mask, beyond the four BarStatusMsg::Field bits.
constexpr int kAllBarStatusFields = 0xff;

// The powerup PostBarStatusMsg() reports for a bar the track description does not enable.
constexpr int kNoPowerbar = -1;

// What Phrase::AddGem() returns when the addition replaced no gem.
constexpr int kNoReplacedGem = -1;

// PostDurGemMsg() joins a gem to the next when the next starts within this many ticks.
constexpr int kJoinTicks = 480;

// The length PostDurGemMsg() gives a gem that is not joined to the next.
constexpr int kSingleGemTicks = 120;

// The blend PostDurGemMsg() starts a run with.
constexpr float kRunStartBlend = 0.5f;

// The gem PostDurGemMsg() posts at the head of each run.
constexpr int kRunHeadGem = 1;

// The ghost flag of the GemMsg objects AddGem() posts for a riff track's other gems.
constexpr int kGhostGem = 1;

// One bar at 480 ticks per quarter note. ReplayBar() uses it rather than mBarTicks.
constexpr int kBarTicks = 1920;

// The bars StartCommands() schedules the two commands for.
constexpr int kFirstBar = 0;
constexpr int kFirstExportBar = 1;

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

    // NTSC-U/C: 0x001bfed8, PAL: 0x001c5d08
    virtual int CmdID() {
        return sCmdID;
    }

    // NTSC-U/C: 0x001bfee8, PAL: 0x001c5d18
    virtual void Execute() {
        mOwner->OnCommand(mBar);
    }

    // NTSC-U/C: 0x001bff08, PAL: 0x001c5d38
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

    // NTSC-U/C: 0x001bffb0, PAL: 0x001c5de0
    virtual int CmdID() {
        return sCmdID;
    }

    // NTSC-U/C: 0x001bffc0, PAL: 0x001c5df0
    virtual void Execute() {
        mOwner->OnExportCommand(mBar);
    }

    // NTSC-U/C: 0x001bffe0, PAL: 0x001c5e10
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

// NTSC-U/C: 0x001ba0d0, PAL: 0x001bfea8
PhraseMgr::PhraseMgr(
    Sch::TickClock *pClock, int nBarTicks, PlayMap *pMap, int nConfig, const TrackData *pTrackData)
    : mPhrasePlayer(nullptr), mNetSink(nullptr), mTrackData(pTrackData), mMap(pMap),
      mPowerbarMgr(nullptr), mTrack(pTrackData->mIndex), mBarTicks(nBarTicks), mConfig(nConfig),
      mWindowStart(0), mWindowEnd(0), mRefreshing(0), mExportLead(0), mClock(pClock),
      mTrackKind(pTrackData->mKind) {
    mExportCommand.mValue = kUnallocatedCommand;
    mCommand.mValue = kUnallocatedCommand;
    mPlayMode = Application::shared()->GetPlayMode();
    mDatabase = new PhraseDatabase(pMap);
    CreatePowerbarMgr();
}

// NTSC-U/C: 0x001ba2b8, PAL: 0x001c0090
PhraseMgr::~PhraseMgr() {
    WithdrawCommands();
    delete mDatabase;
    delete mPowerbarMgr;
}

// NTSC-U/C: 0x001ba3d8, PAL: 0x001c01b0
void PhraseMgr::CreatePowerbarMgr() {
    delete mPowerbarMgr;
    mPowerbarMgr = nullptr;

    const int nGameMode = Application::shared()->GetGameMode();
    if (mPlayMode == kPlayModeGame &&
        (mTrackKind == kTrackModeCatch || mTrackKind == kTrackModeRiff) &&
        !QueryConfigFlag(kDisplayModeQuery)) {
        if (nGameMode == kGameModeSolo) {
            mPowerbarMgr = new SoloPowerbarMgr(mMap, mDatabase, mTrackData, mTrack);
        } else {
            mPowerbarMgr = new MultiPowerbarMgr(mMap, mDatabase, mTrackData, mTrack);
        }
        return;
    }
    mPowerbarMgr = new JamPowerbarMgr;
}

inline void PhraseMgr::RefreshWindowBarOfStep(int nStep) {
    for (int nBar = mWindowStart; nBar < mWindowEnd; ++nBar) {
        if (mMap->MapBar(nBar) == nStep) {
            RefreshBar(nBar, 1);
            break;
        }
    }
}

// NTSC-U/C: 0x001ba540, PAL: 0x001c0318
void PhraseMgr::OnCaughtPhrasePacket(Message *pMsg) {
    CaughtPhrasePacket *pPacket = static_cast<CaughtPhrasePacket *>(pMsg);
    if (static_cast<int>(pPacket->mTr) != mTrack) {
        return;
    }

    const int nFirstStep = pPacket->mB;
    int nStep = nFirstStep;
    do {
        Player *pPrevious = mDatabase->GetOwner(nStep);
        Player *pPlayer = pPacket->mPlayer;
        if (pPlayer->IsNull() == 0) {
            mDatabase->SetOwner(pPlayer, nStep);
        } else {
            mDatabase->ClearPhrase(nStep);
        }
        if (pPrevious != pPlayer) {
            mTrackData->SetOwner(pPrevious, nStep);
        }
        RefreshWindowBarOfStep(nStep);
        nStep = mMap->MapToLinkedStep(nStep, mTrack);
    } while (nStep != nFirstStep);
}

// NTSC-U/C: 0x001ba6d0, PAL: 0x001c04a8
void PhraseMgr::PostGemMsg(Message *pMsg) {
    GemPacket *pPacket = static_cast<GemPacket *>(pMsg);
    if (pPacket->mTr != mTrack) {
        return;
    }

    const GemPacket::Fields &gem = pPacket->mFields;
    const int nFirstStep = gem.mBar;
    int nStep = nFirstStep;
    do {
        Phrase *pPhrase = mDatabase->GetPhrase(nStep);
        if (pPhrase == nullptr) {
            mDatabase->SetOwner(gem.mPlayer, nStep);
            mTrackData->SetOwner(&g_nullPlayer, nStep);
            pPhrase = mDatabase->GetPhrase(nStep);
        }
        pPhrase->AddGem(gem.mLoc.mTick, gem.mGem, gem.mTrans);

        for (int nBar = mWindowStart; nBar < mWindowEnd; ++nBar) {
            if (mMap->MapBar(nBar) == nStep) {
                const Mid::MBT start(ClampPosition(mBarTicks * nBar));
                GemMsg msg(Mid::MBT(ClampPosition(gem.mLoc.mTick + start.mTick)),
                           mTrack,
                           gem.mGem,
                           gem.mPlayer);
                Send(&msg);
                break;
            }
        }
        nStep = mMap->MapToLinkedStep(nStep, mTrack);
    } while (nStep != nFirstStep);
}

// NTSC-U/C: 0x001ba928, PAL: 0x001c0700
void PhraseMgr::OnRefreshNet(Message *pMsg) {
    RefreshNetMsg *pRefresh = static_cast<RefreshNetMsg *>(pMsg);
    if (pRefresh->mTrack != mTrack || mNetSink == nullptr) {
        return;
    }

    for (int nBar = pRefresh->mFirstBar; nBar < pRefresh->mEndBar; ++nBar) {
        const int nStep = mMap->MapBar(nBar);
        Phrase *pPhrase = mDatabase->GetPhrase(nStep);
        CaughtPhrasePacket packet(
            pPhrase != nullptr ? pPhrase->mPlayer : &g_nullPlayer, mTrack, nStep);
        mNetSink->Handle(&packet);
    }
}

// NTSC-U/C: 0x001baa98, PAL: 0x001c0870
void PhraseMgr::AddGem(int nGem, int nTrans, int nBar, int nTick, Player *pOwner, int bPost) {
    Application::shared()->GetWorld()->MarkStatsFlag();

    const int nStep = mMap->MapBar(nBar);
    Phrase *pPhrase = mDatabase->GetPhrase(nStep);
    if (pPhrase == nullptr) {
        SetPhraseOwner(pOwner, nBar);
        pPhrase = mDatabase->GetPhrase(nStep);
    }
    const int nReplaced = pPhrase->AddGem(nTick, nGem, nTrans);

    if (mNetSink != nullptr) {
        GemPacket::Fields fields;
        fields.mGem = nGem;
        // Yes, the binary leaves fields.mTrans unset.
        fields.mBar = nStep;
        fields.mLoc.mTick = nTick;
        fields.mPlayer = pOwner;
        GemPacket packet(fields, mTrack);
        mNetSink->Handle(&packet);
    }

    if (bPost != 0) {
        const std::vector<int> &bars = mMap->FindBarsPlaying(nStep, nBar, mWindowEnd);
        for (std::vector<int>::const_iterator it = bars.begin(); it != bars.end(); ++it) {
            const int nWindowBar = *it;
            const Mid::MBT start(ClampPosition(mBarTicks * nWindowBar));
            const Mid::MBT position(ClampPosition(nTick + start.mTick));
            if (nReplaced != kNoReplacedGem) {
                {
                    ClearGemMsg clear(position, mTrack, nReplaced);
                    Send(&clear);
                }
                if (mTrackKind == kTrackModeRiff) {
                    // Repost the first other gem the track lists at the same position, as a ghost.
                    const std::vector<TickObj<int> > &gems = *mTrackData->GetGems(nWindowBar);
                    for (std::vector<TickObj<int> >::const_iterator other = gems.begin();
                         other != gems.end();
                         ++other) {
                        if (other->mPosition.mTick == nTick && other->mValue != nGem) {
                            const Mid::MBT otherStart(ClampPosition(mBarTicks * nWindowBar));
                            GemMsg ghost(
                                Mid::MBT(ClampPosition(other->mPosition.mTick + otherStart.mTick)),
                                mTrack,
                                other->mValue,
                                pOwner,
                                kGhostGem);
                            Send(&ghost);
                            break;
                        }
                        if (nTick < other->mPosition.mTick) {
                            break;
                        }
                    }
                }
            }
            GemMsg msg(position, mTrack, nGem, pOwner);
            Send(&msg);
        }
    }
    (void)mMap->MapToLinkedStep(nStep, mTrack); // Yes, the binary discards this step.
}

// NTSC-U/C: 0x001bafa8, PAL: 0x001c0d80
void PhraseMgr::SetPhraseOwner(Player *pPlayer, int nBar) {
    const int nFirstStep = mMap->MapBar(nBar);
    int nStep = nFirstStep;
    do {
        Player *pPrevious = mDatabase->GetOwner(nStep);
        mDatabase->SetOwner(pPlayer, nStep);
        if (pPrevious != pPlayer) {
            // The previous owner, not the new one. That is what the binary passes.
            mTrackData->SetOwner(pPrevious, nStep);
        }
        if (mNetSink != nullptr) {
            CaughtPhrasePacket packet(pPlayer, mTrack, nStep);
            mNetSink->Handle(&packet);
        }

        const std::vector<int> &bars = mMap->FindBarsPlaying(nStep, nBar, mWindowEnd);
        for (std::vector<int>::const_iterator it = bars.begin(); it != bars.end(); ++it) {
            RefreshBar(*it, 0);
        }
        nStep = mMap->MapToLinkedStep(nStep, mTrack);
    } while (mPlayMode == kPlayModeGame && nStep != nFirstStep);
}

// NTSC-U/C: 0x001bb1a0, PAL: 0x001c0f78
void PhraseMgr::InstallPhrase(Phrase *pPhrase, int nBar, int bRefresh) {
    Application::shared()->GetWorld()->MarkStatsFlag();

    const int nStep = mMap->MapBar(nBar);
    Player *pPrevious = mDatabase->GetOwner(nStep);
    mDatabase->SetPhrase(pPhrase, nStep);
    if (pPrevious != pPhrase->mPlayer) {
        mTrackData->SetOwner(pPrevious, nStep);
    }
    if (mNetSink != nullptr) {
        CaughtPhrasePacket packet(pPhrase->mPlayer, mTrack, nStep);
        mNetSink->Handle(&packet);
    }
    if (bRefresh != 0) {
        RefreshBar(nBar, 0);
    }
}

// NTSC-U/C: 0x001bb328, PAL: 0x001c1100
void PhraseMgr::ClearPhrase(int nBar, int bAll) {
    Application::shared()->GetWorld()->MarkStatsFlag();

    const int nFirstStep = mMap->MapBar(nBar);
    int nStep = nFirstStep;
    do {
        Player *pPrevious = mDatabase->GetOwner(nStep);
        mDatabase->ClearPhrase(nStep);
        if (pPrevious->IsNull() == 0) {
            mTrackData->SetOwner(pPrevious, nStep);
        }
        if (mNetSink != nullptr) {
            CaughtPhrasePacket packet(&g_nullPlayer, mTrack, nStep);
            mNetSink->Handle(&packet);
        }

        const std::vector<int> &bars = mMap->FindBarsPlaying(nStep, nBar, mWindowEnd);
        for (std::vector<int>::const_iterator it = bars.begin(); it != bars.end(); ++it) {
            RefreshBar(*it, 1);
        }
        nStep = mMap->MapToLinkedStep(nStep, mTrack);
    } while (mPlayMode == kPlayModeGame && bAll != 0 && nStep != nFirstStep);
}

// NTSC-U/C: 0x001bb558, PAL: 0x001c1330
int PhraseMgr::PhrasesMatch(int nFirstBar, int nSecondBar) {
    if (nFirstBar == nSecondBar) {
        return 1;
    }

    const int nFirstStep = mMap->MapBar(nFirstBar);
    const int nSecondStep = mMap->MapBar(nSecondBar);
    Phrase *pFirst = mDatabase->GetPhrase(nFirstStep);
    Phrase *pSecond = mDatabase->GetPhrase(nSecondStep);
    if (pFirst == nullptr) {
        return pSecond == nullptr;
    }
    if (pSecond == nullptr) {
        return 0;
    }
    return pFirst->mGems == pSecond->mGems;
}

// NTSC-U/C: 0x001bb798, PAL: 0x001c1570
void PhraseMgr::ReplayBar(int nBar, int nOffset) {
    const int nNow = mClock->SongTick();
    const Mid::MBT start(ClampPosition(nBar * Mid::MBT(kBarTicks).mTick));
    const Mid::MBT elapsed(ClampPosition(nNow - start.mTick));
    mPhrasePlayer->PlayBarAt(nBar, nOffset, elapsed.mTick);
}

// NTSC-U/C: 0x001bb6b8, PAL: 0x001c1490
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

// NTSC-U/C: 0x001bb8a0, PAL: 0x001c1678
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

// NTSC-U/C: 0x001bb9f8, PAL: 0x001c17d0
void PhraseMgr::PostBarStatusMsg(int nBar) {
    const int nStep = mMap->MapBar(nBar);
    (void)Mid::MBT(ClampPosition(mBarTicks * nBar)); // Yes, the binary discards this position.
    const int nEnabled = mTrackData->QueryBar(nBar);
    (void)mTrackData->GetQuant(nBar); // Yes, the binary discards this result.
    const int nPowerup = nEnabled != 0 ? mPowerbarMgr->GetPowerbar(nStep) : kNoPowerbar;
    const long long *pEffects = mDatabase->GetStepValue(nBar);
    Phrase *pPhrase = mDatabase->GetPhraseAt(nBar);

    BarStatusMsg msg;
    msg.mBar = nBar;
    msg.mTrack = mTrack;
    msg.mPlayer = pPhrase != nullptr ? pPhrase->mPlayer : &g_nullPlayer;
    msg.mEnabled = nEnabled;
    msg.mRefreshing = mRefreshing;
    msg.mPowerup = nPowerup;
    msg.mEffects = BarStatusMsg::Effects(*pEffects);
    msg.mFlags = kAllBarStatusFields;
    Send(&msg);
}

// NTSC-U/C: 0x001bbb90, PAL: 0x001c1968
void PhraseMgr::RefreshBar(int nBar, int bClear) {
    if (nBar < mWindowStart || nBar >= mWindowEnd) {
        return;
    }

    if (bClear != 0) {
        ClearGemsMsg clear;
        clear.mBar = nBar;
        clear.mTrack = mTrack;
        Send(&clear);
    }
    PostBarStatusMsg(nBar);

    switch (mTrackKind) {
    case kTrackModeAxe:
        PostPhraseMsg(nBar);
        break;
    case kTrackModeRiff:
        if (!Application::shared()->IsJukeboxMode()) {
            PostGemMsgThird(nBar, 1);
        }
        PostGemMsgSecond(nBar);
        break;
    case kTrackModeScratch:
        PostDurGemMsg(nBar);
        break;
    case kTrackModeVocal:
        PostPhraseMsg(nBar); // The binary calls the byte-identical copy at 0x001bc4f8.
        break;
    case kTrackModeCatch:
        PostGemMsgThird(nBar, 0);
        break;
    default:
        break;
    }
}

// NTSC-U/C: 0x001bbcf0, PAL: 0x001c1ac8
void PhraseMgr::PostDurGemMsg(int nBar) {
    Phrase *pPhrase = mDatabase->GetPhraseAt(nBar);
    if (pPhrase == nullptr) {
        return;
    }

    float flPreviousBlend = kRunStartBlend;
    int bJoinedToPrevious = 0;
    for (std::vector<Phrase::Gem>::iterator it = pPhrase->mGems.begin(); it != pPhrase->mGems.end();
         ++it) {
        const std::vector<Phrase::Gem>::iterator next = it + 1;
        int bJoinedToNext = 0;
        if (next != pPhrase->mGems.end()) {
            const Mid::MBT joinEnd(ClampPosition(it->mPosition.mTick + Mid::MBT(kJoinTicks).mTick));
            if (next->mPosition.mTick < joinEnd.mTick) {
                bJoinedToNext = next->mTrans * it->mTrans < 0;
            }
        }

        const int nStartTick = it->mPosition.mTick;
        const float flEndBlend = AxeOldGemMaker::BlendForStep(it->mTrans);
        int nEndTick = Mid::MBT(ClampPosition(nStartTick + Mid::MBT(kSingleGemTicks).mTick)).mTick;
        const float flStartBlend = bJoinedToPrevious != 0 ? flPreviousBlend : kRunStartBlend;
        if (bJoinedToNext != 0) {
            nEndTick = next->mPosition.mTick;
            (void)AxeOldGemMaker::BlendForStep(it->mTrans); // Yes, the binary discards this result.
        }

        const Mid::MBT barStart(ClampPosition(mBarTicks * nBar));
        if (it->mTrans != 0) {
            DurGemMsg msg;
            msg.mLane = mTrack;
            msg.mStartFrame = Mid::MBT(ClampPosition(barStart.mTick + nStartTick)).mTick;
            msg.mStartBlend = flStartBlend;
            msg.mEndFrame = Mid::MBT(ClampPosition(barStart.mTick + nEndTick)).mTick;
            msg.mEndBlend = flEndBlend;
            msg.mLive = 0;
            msg.mPlayer = pPhrase->mPlayer;
            Send(&msg);
        }
        if (bJoinedToPrevious == 0) {
            GemMsg msg(Mid::MBT(ClampPosition(barStart.mTick + nStartTick)),
                       mTrack,
                       kRunHeadGem,
                       pPhrase->mPlayer);
            Send(&msg);
        }

        flPreviousBlend = flEndBlend;
        bJoinedToPrevious = bJoinedToNext;
    }
}

// NTSC-U/C: 0x001bc0f0, PAL: 0x001c1ec8
void PhraseMgr::PostGemMsgSecond(int nBar) {
    Phrase *pPhrase = mDatabase->GetPhraseAt(nBar);
    if (pPhrase == nullptr) {
        return;
    }

    for (std::vector<Phrase::Gem>::const_iterator gem = pPhrase->mGems.begin();
         gem != pPhrase->mGems.end();
         ++gem) {
        const Mid::MBT start(ClampPosition(mBarTicks * nBar));
        GemMsg msg(Mid::MBT(ClampPosition(start.mTick + gem->mPosition.mTick)),
                   mTrack,
                   gem->mGem,
                   pPhrase->mPlayer);
        Send(&msg);
    }
}

// NTSC-U/C: 0x001bc290, PAL: 0x001c2068
void PhraseMgr::PostGemMsgThird(int nBar, int bGhost) {
    if (mTrackData->QueryBar(nBar) == 0) {
        return;
    }

    Player *pPlayer = &g_nullPlayer;
    if (bGhost == 0) {
        Phrase *pPhrase = mDatabase->GetPhraseAt(nBar);
        if (pPhrase != nullptr) {
            pPlayer = pPhrase->mPlayer;
        }
    }

    const std::vector<TickObj<int> > &gems = *mTrackData->GetGems(nBar);
    for (std::vector<TickObj<int> >::const_iterator gem = gems.begin(); gem != gems.end(); ++gem) {
        const Mid::MBT start(ClampPosition(mBarTicks * nBar));
        GemMsg msg(Mid::MBT(ClampPosition(gem->mPosition.mTick + start.mTick)),
                   mTrack,
                   gem->mValue,
                   pPlayer,
                   bGhost);
        Send(&msg);
    }
}

// NTSC-U/C: 0x001bc468, PAL: 0x001c2240
void PhraseMgr::PostPhraseMsg(int nPhrase) {
    Phrase *pPhrase = mDatabase->GetPhraseAt(nPhrase);
    if (pPhrase == nullptr) {
        return;
    }

    PhraseMsg msg;
    msg.mBar = nPhrase;
    msg.mTrack = mTrack;
    msg.mPhrase = pPhrase;
    Send(&msg);
}

// NTSC-U/C: 0x001bc588, PAL: 0x001c2360
void PhraseMgr::StartCommands() {
    Cmd *pCommand = new Cmd(this, kFirstBar);
    const Mid::MBT when(ClampPosition(mBarTicks * kFirstBar));
    mClock->PostAtSongTick(pCommand, when.mTick, mCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }

    ExportCmd *pExportCommand = new ExportCmd(this, kFirstExportBar);
    const Mid::MBT start(ClampPosition(mBarTicks * kFirstExportBar));
    const Mid::MBT exportWhen(ClampPosition(start.mTick + mExportLead.mTick));
    mClock->PostAtSongTick(pExportCommand, exportWhen.mTick, mExportCommand);
    if (pExportCommand != nullptr) {
        pExportCommand->Release();
    }
}

// NTSC-U/C: 0x001bc718, PAL: 0x001c24f0
void PhraseMgr::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nPhrasePacketType) {
        OnPhrasePacket(static_cast<PhrasePacket *>(pMsg));
    } else if (nType == g_nCaughtPhrasePacketType) {
        OnCaughtPhrasePacket(pMsg);
    } else if (nType == g_nGemPacketType) {
        PostGemMsg(pMsg);
    } else if (nType == g_nInvalidateTrackMsgType) {
        OnInvalidateTrack(static_cast<InvalidateTrackMsg *>(pMsg));
    } else if (nType == g_nRefreshNetMsgType) {
        OnRefreshNet(pMsg);
    } else if (nType == g_nGameBeginMsgType) {
        RefreshAllBars();
    }
}

// NTSC-U/C: 0x001bf6c0, PAL: 0x001c54e0
int PhraseMgr::TickToBar(int nTick) {
    const int nBar = nTick / mBarTicks;
    (void)Mid::MBT(ClampPosition(mBarTicks * nBar)); // Yes, the binary discards this position.
    return nBar;
}

// NTSC-U/C: 0x001bf738, PAL: 0x001c5558
int PhraseMgr::BarToTick(int nBar) {
    return Mid::MBT(ClampPosition(mBarTicks * nBar)).mTick;
}

// NTSC-U/C: 0x001c0010, PAL: 0x001c5e40
void PhraseMgr::OnPhrasePacket(PhrasePacket *pPacket) {
    if (static_cast<int>(pPacket->mTr) != mTrack) {
        return;
    }

    const int nStep = pPacket->mB;
    Phrase *pPhrase = pPacket->mPhrase;
    Player *pPrevious = mDatabase->GetOwner(nStep);
    Player *pOwner = pPhrase != nullptr ? pPhrase->mPlayer : &g_nullPlayer;
    if (pPhrase != nullptr) {
        mDatabase->SetPhrase(pPhrase, nStep);
    } else {
        mDatabase->ClearPhrase(nStep);
    }
    if (pPrevious != pOwner) {
        mTrackData->SetOwner(pPrevious, nStep);
    }
    RefreshWindowBarOfStep(nStep);
}

// NTSC-U/C: 0x001c0110, PAL: 0x001c5f40
void PhraseMgr::OnInvalidateTrack(InvalidateTrackMsg *pMsg) {
    if (pMsg->mTrack != mTrack) {
        return;
    }

    const int nFirstStep = pMsg->mFirstBar;
    const int nEndStep = pMsg->mEndBar;
    for (int nBar = mWindowStart; nBar < mWindowEnd; ++nBar) {
        const int nStep = mMap->MapBar(nBar);
        if (nStep >= nFirstStep && nStep < nEndStep) {
            RefreshBar(nBar, 1);
        }
    }
}

// NTSC-U/C: 0x001c01c8, PAL: 0x001c5ff8
Phrase *PhraseMgr::GetPhraseAt(int nBar) {
    return mDatabase->GetPhraseAt(nBar);
}

// NTSC-U/C: 0x001c01e8, PAL: 0x001c6018
int PhraseMgr::GetPowerbar(int nBar) {
    return mPowerbarMgr->GetPowerbar(mMap->MapBar(nBar));
}

// NTSC-U/C: 0x001c0248, PAL: 0x001c6078
long long *PhraseMgr::GetStepValue(int nBar) {
    return mDatabase->GetStepValue(nBar);
}

// NTSC-U/C: 0x001c0268, PAL: 0x001c6098
Player *PhraseMgr::GetPhraseOwner(int nBar) {
    Phrase *pPhrase = mDatabase->GetPhraseAt(nBar);
    if (pPhrase == nullptr) {
        return &g_nullPlayer;
    }
    return pPhrase->mPlayer;
}

// NTSC-U/C: 0x001c0298, PAL: 0x001c60c8
void PhraseMgr::SetPhraseByte(int nBar, char cValue) {
    const int nFirst = mMap->MapBar(nBar);
    int nIndex = nFirst;
    do {
        mDatabase->SetPhraseByte(nIndex, cValue);
        nIndex = mMap->MapToLinkedStep(nIndex, mTrack);
    } while (nIndex != nFirst);
}

// NTSC-U/C: 0x001c0338, PAL: 0x001c6168
unsigned char PhraseMgr::GetPhraseByte(int nBar) {
    return mDatabase->GetPhraseByte(mMap->MapBar(nBar));
}

// NTSC-U/C: 0x001c0380, PAL: 0x001c61b0
void PhraseMgr::ResetOwners(Player *pPlayer) {
    mDatabase->SetOwners(pPlayer);
    for (int nBar = mWindowStart; nBar < mWindowEnd; ++nBar) {
        RefreshBar(nBar, 1);
    }
}

// NTSC-U/C: 0x001c03e0, PAL: 0x001c6210
void PhraseMgr::RefreshAllBars() {
    mRefreshing = 1;
    mWindowStart = 0;
    mWindowEnd = mConfig + 1;
    for (int nBar = 0; nBar < mWindowEnd; ++nBar) {
        RefreshBar(nBar, 0);
    }
    mRefreshing = 0;
}

// NTSC-U/C: 0x001c0450, PAL: 0x001c6280
void PhraseMgr::WithdrawCommands() {
    mClock->Withdraw(mCommand);
    mClock->Withdraw(mExportCommand);
}
