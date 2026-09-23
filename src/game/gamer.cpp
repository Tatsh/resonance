#include "game/gamer.h"

#include <algorithm>

#include "app/application.h"
#include "app/playsound.h"
#include "game/bgtrackgraph.h"
#include "game/enablemgr.h"
#include "game/gameenablemgr.h"
#include "game/gamemanagerimpl.h"
#include "game/gamercmd.h"
#include "game/gamestats.h"
#include "game/grooveworld.h"
#include "game/inputmap.h"
#include "game/leveldata.h"
#include "game/localjamenablemgr.h"
#include "game/netjamenablemgr.h"
#include "game/phrasedatabase.h"
#include "game/player.h"
#include "game/playmap.h"
#include "game/scoretrackgraph.h"
#include "game/trackdata.h"
#include "msg/advancesectiontogglemsg.h"
#include "msg/freestylefxmsg.h"
#include "msg/invalidateseekermsg.h"
#include "msg/invalidatetrackmsg.h"
#include "msg/tracksonmsg.h"
#include "msg/winmsg.h"
#include "os/log.h"
#include "sch/tickclock.h"
#include "script/configquery.h"
#include "script/scripthost.h"
#include "synth/midi_main.h"

namespace {

// Configuration code of the solo game's track requirement lists.
constexpr int kSoloRequirementsConfigCode = 0x387;

// Configuration codes of the constructor's recorded values.
constexpr int kUnknown3cConfigCode = 0x2be;
constexpr int kSoloJuiceConfigCode = 0x38c;
constexpr int kSoloMaxJuiceConfigCode = 0x394;
constexpr int kTutorialConfigCode = 0x3a1;

// MIDI ticks in one bar.
constexpr int kTicksPerBar = 1920;

// The values the constructor starts its words at.
constexpr int kInitialUnknown44 = 2;
constexpr int kInitialFreeEndBar = -1;
constexpr int kUnallocatedCommand = -2;

// The script template AdvanceTo() runs.
constexpr int kAdvanceScriptTemplate = 1013;

// The sounds a game's end plays.
constexpr char kWinSound[] = "SND_WIN";
constexpr char kLoseSound[] = "SND_LOSE";

// Bars after the last one at which a finished multiplayer game exits.
constexpr int kExitDelayBars = 4;

// The span a solo win passes to Player::Slot8().
constexpr int kWonBarSpan = 100000;

// SetFreeUntil()'s end bar that frees a track for good.
constexpr int kFreeForever = -1;

// A solo player below this juice cannot continue.
constexpr int kMinimumJuice = 2;

// The juice a bar costs.
constexpr int kBarJuiceCost = -1;

// Bars the streamed audio runs behind the update.
constexpr int kSynthStreamLeadBars = 3;

// Configuration code of the background tracks' enable policy.
constexpr int kBackTrackConfigCode = 0x386;

// The score ceiling EndWithScore() sets.
constexpr int kEndScoreCeiling = 10000;

constexpr char kFreestyleOutsideTutorial[] =
    " Only call Gamer::EnablePlayerFreestyle() from tutorial.";

// The progress a completed solo song records.
constexpr float kCompleteProgress = 1.0f;

// The score and ceiling every player starts with.
constexpr int kInitialScore = 0;
constexpr int kMaxScore = 100000;

// The slot whose rotations a jukebox session keeps on.
constexpr int kJukeboxSlot = 0;

// The owner buckets a network jam lists as open.
constexpr int kNetJamBucketCount = 8;

// A network jam with at least this many players shares the tracks among them.
constexpr unsigned kSharedTrackPlayerCount = 2;

} // namespace

Gamer::Gamer(int nTrackCount, int nEndBar, GameStats *pStats)
    : mUnknown1c(0), mEndState(kEndStateNone), mJukeboxMode(0), mUnknown38(0),
      mTrackCount(nTrackCount), mUnknown44(kInitialUnknown44), mUnknown48(0), mPlaybackOn(0),
      mGlobals(Application::shared()), mStats(pStats), mBarLength(kTicksPerBar),
      mPlayers(mGlobals->GetWorld()->mPlayers), mBackGraphs(nullptr), mGraphs(nullptr),
      mTrackSources(nTrackCount, MsgSource()), mUnknown84(0), mEnableMgr(nullptr),
      mBackEnableMgr(nullptr), mUnknown98(0) {
    mCommand.mValue = kUnallocatedCommand;
    mFreeEndBar = kInitialFreeEndBar;
    mPlayMap = mGlobals->GetPlayMap();
    mEndBar = nEndBar;
    mUnknown3c = QueryConfigValue(kUnknown3cConfigCode);
    mGameMode = mGlobals->GetGameMode();
    mPlayMode = mGlobals->GetPlayMode();
    mJukeboxMode = mGlobals->IsJukeboxMode();

    int nJuice = 0;
    int nMaxJuice = 0;
    if (mGameMode == kGameModeSolo) {
        nJuice = QueryConfigValue(kSoloJuiceConfigCode);
        nMaxJuice = QueryConfigValue(kSoloMaxJuiceConfigCode);
    }
    mTutorial = QueryConfigFlag(kTutorialConfigCode);
    for (std::vector<Player *>::iterator it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        (*it)->SetScore(kInitialScore, kMaxScore);
        (*it)->SetJuice(nJuice, nMaxJuice);
    }

    if (mJukeboxMode != 0) {
        InputMap *pInputMap = InputMap::shared();
        mPlaybackOn = 1;
        pInputMap->DisableEntries();
        pInputMap->SetEnabled(kJukeboxSlot, InputMap::kActionRotateLeft, 1);
        pInputMap->SetEnabled(kJukeboxSlot, InputMap::kActionRotateRight, 1);
    }
}

Gamer::~Gamer() {
    Withdraw();
    delete mEnableMgr;
    delete mBackEnableMgr;
}

void Gamer::Withdraw() {
    const CmdID command = mCommand;
    mGlobals->GetSongClock()->Withdraw(command);
}

void Gamer::CreateEnableMgr(std::vector<ScoreTrackGraph *> *pGraphs) {
    mGraphs = pGraphs;
    if (mPlayMode == kPlayModeGame) {
        if (mGameMode == kGameModeSolo) {
            mEnableMgr =
                GameEnableMgr::CreateReleasing(kSoloRequirementsConfigCode, mTrackCount, this);
        } else if (mGameMode == kGameModeLocal || mGameMode == kGameModeNet) {
            mEnableMgr = GameEnableMgr::CreateUnrestricted(mTrackCount, this);
        }
        for (int i = 0; i < mTrackCount; ++i) {
            if (IsNonCatchTrack(i)) {
                mEnableMgr->DisableTrack(i);
            }
        }
    } else if (mGameMode == kGameModeSolo) {
        mEnableMgr = LocalJamEnableMgr::CreateSolo();
    } else if (mGameMode == kGameModeLocal) {
        mEnableMgr = LocalJamEnableMgr::CreateLocal();
    } else if (mGameMode == kGameModeNet) {
        const std::vector<Player *> &players = mGlobals->GetWorld()->mPlayers;
        int nMaxOwned = kNetJamBucketCount;
        if (players.size() >= kSharedTrackPlayerCount) {
            nMaxOwned = (mTrackCount + 1) / static_cast<int>(players.size());
        }

        std::vector<int> openTracks;
        for (int i = 0; i < kNetJamBucketCount; ++i) {
            openTracks.push_back(i);
        }
        mEnableMgr = NetJamEnableMgr::Create(mTrackCount, nMaxOwned, &openTracks, this);
    }
}

void Gamer::SetBarOwner(int nTrack, int nBar, Player *pPlayer) {
    mEnableMgr->SetBarOwner(nTrack, nBar, pPlayer);
}

int Gamer::QueryBar(int nTrack, int nBar) {
    return mEnableMgr->QueryBar(nTrack, nBar);
}

bool Gamer::IsNonCatchTrack(int nTrack) {
    return GetTrack(nTrack)->mKind != kTrackModeCatch;
}

PhraseDatabase *Gamer::GetPhraseDatabase(int nTrack) {
    return (*mGraphs)[nTrack]->GetPhraseDatabase();
}

TrackData *Gamer::GetTrack(int nTrack) {
    return mGlobals->GetLevel()->TrackAt(nTrack);
}

void Gamer::AdvanceTo(int nBar, int nAdvance) {
    const Mid::MBT position(
        std::min(std::max(nBar * Mid::MBT(kTicksPerBar).mTick, kMBTMinimum), kMBTMaximum));
    AdvanceSectionToggleMsg toggle(nAdvance, position);
    Send(&toggle);

    const int nStart = mPlayMap->Slot5(mPlayMap->FollowingStepBar(nBar));
    const int nEnd = nStart + mUnknown3c;
    for (int i = 0; i < mTrackCount; ++i) {
        InvalidateTrackMsg invalidateTrack(nStart, nEnd, i);
        mTrackSources[i].Send(&invalidateTrack);
        InvalidateSeekerMsg invalidateSeeker(nBar, i);
        mTrackSources[i].Send(&invalidateSeeker);
    }

    CallScriptTemplate(kAdvanceScriptTemplate);
}

void Gamer::AdvanceAt(Mid::MBT position) {
    const int nBar = position.mTick / mBarLength.mTick;
    AdvanceTo(nBar, mPlayMap->Slot18(nBar));
}

bool Gamer::SendTracksOn(int nBar) {
    int nOwnedTracks = 0;
    int nOpenTracks = 0;
    const int nStep = mPlayMap->Slot5(nBar);
    for (int i = 0; i < mTrackCount; ++i) {
        TrackData *pTrack = GetTrack(i);
        if (pTrack->mKind != kTrackModeCatch) {
            continue;
        }

        if (GetPhraseDatabase(i)->GetOwner(nStep)->IsNull() == 0) {
            ++nOwnedTracks;
        } else if (!pTrack->GetGemsInBar(nStep)->empty() && mEnableMgr->QueryBar(i, nBar) != 0) {
            ++nOpenTracks;
        }
    }

    TracksOnMsg msg(nBar, nOwnedTracks);
    Send(&msg);
    return nOpenTracks == 0;
}

bool Gamer::FreeTracksAfterCapture(int nBar) {
    const bool bComplete = SendTracksOn(nBar);
    if (!bComplete) {
        return bComplete;
    }

    const int nNextBar = mPlayMap->FollowingStepBar(nBar);
    mPlayers[0]->Slot8(nBar, nNextBar);
    mUnknown84 = nBar + 1;

    int nFreeEndBar = mFreeEndBar;
    for (int i = 0; i < mTrackCount; ++i) {
        if (!IsNonCatchTrack(i) || mFreeEndBar >= nNextBar) {
            continue;
        }

        const int nStartBar = std::max(nBar, mFreeEndBar);
        mEnableMgr->SetFreeUntil(i, nStartBar, nNextBar);
        FreestyleFXMsg msg(i, nStartBar, nNextBar);
        Send(&msg);
        nFreeEndBar = nNextBar;
    }
    mFreeEndBar = nFreeEndBar;
    return bComplete;
}

void Gamer::DeclareWinners() {
    int nBestScore = 0;
    for (unsigned i = 0; i < mPlayers.size(); ++i) {
        const int nScore = mPlayers[i]->GetScore();
        if (nBestScore < nScore) {
            nBestScore = nScore;
        }
        mStats->SetScore(i, nScore);
    }

    WinMsg win;
    for (unsigned i = 0; i < mPlayers.size(); ++i) {
        if (mPlayers[i]->GetScore() == nBestScore) {
            win.AddWinner(mPlayers[i]);
        }
    }
    Send(&win);
    PlaySoundByName(kWinSound);
    mEndState = kEndStateOver;
}

void Gamer::RecordSoloStats(int bCompleted, int nBar) {
    mStats->mCompleted = bCompleted;
    mStats->mUnknown08 = mUnknown98;
    mStats->SetScore(0, mPlayers[0]->GetScore());
    if (bCompleted != 0) {
        mStats->SetProgress(kCompleteProgress);
    } else {
        mStats->SetProgress(static_cast<float>(nBar) / static_cast<float>(mPlayMap->Slot9()));
    }
    mStats->SetTally(0, mPlayers[0]->Slot17());
    mStats->SetRatio(0, mPlayers[0]->Slot18());
}

void Gamer::OnBar(int nBar) {
    mPlayMap->Slot5(nBar); // Yes, the binary discards this call's result.
    mUnknown38 = nBar;
    for (unsigned i = 0; i < mBackGraphs->size(); ++i) {
        if (mBackEnableMgr->QueryBar(i, nBar) != 0) {
            (*mBackGraphs)[i]->DisableMidi();
        } else {
            (*mBackGraphs)[i]->EnableMidi();
        }
    }

    if (mPlayMode == kPlayModeGame && mTutorial == 0) {
        if (mGameMode != kGameModeSolo) {
            if (nBar == mEndBar && mEndState == kEndStateNone) {
                DeclareWinners();
            }
            if (nBar == mEndBar + kExitDelayBars && mEndState == kEndStateOver) {
                mGlobals->GetWorld()->PostExitMode1();
            }
        } else {
            Player *pPlayer = mPlayers[0];
            pPlayer->Slot2(); // Yes, the binary discards this call's result.
            if (nBar >= mEndBar && mEndState == kEndStateNone) {
                mEndState = kEndStateWon;
                pPlayer->Slot8(nBar, nBar + kWonBarSpan);

                WinMsg win;
                win.AddWinner(pPlayer);
                Send(&win);
                PlaySoundByName(kWinSound);
                RecordSoloStats(1, nBar);

                TracksOnMsg tracksOn(nBar, 0);
                Send(&tracksOn);
                for (int i = 0; i < mTrackCount; ++i) {
                    if (IsNonCatchTrack(i)) {
                        mEnableMgr->SetFreeUntil(i, 0, kFreeForever);
                    } else if ((*mGraphs)[i]->Slot11() != 0) {
                        (*mGraphs)[i]->Slot10(nBar, pPlayer);
                    }
                }
            } else if (pPlayer->GetJuice() < kMinimumJuice) {
                if (mEndState != kEndStateNone) {
                    InputMap::shared()->EnableEntries();
                    mGlobals->GetWorld()->PostExitMode1();
                } else {
                    bool bExhausted = true;
                    for (int i = 0; i < mTrackCount; ++i) {
                        if ((*mGraphs)[i]->Slot9() == 0) {
                            bExhausted = false;
                            break;
                        }
                    }

                    if (bExhausted) {
                        WinMsg lose;
                        Send(&lose);
                        mEndState = kEndStateOver;
                        if (mUnknown1c == 0) {
                            pPlayer->AddJuice(kBarJuiceCost, 1);
                        }
                        PlaySoundByName(kLoseSound);
                        RecordSoloStats(0, nBar);
                        InputMap::shared()->DisableEntries();
                    }
                }
            } else if (mEndState == kEndStateNone && !FreeTracksAfterCapture(nBar) &&
                       mUnknown1c == 0) {
                pPlayer->AddJuice(kBarJuiceCost, 1);
            }
        }
    }

    if ((mPlayMode == kPlayModeJam || mTutorial != 0) && mPlayMap->IsStepStart(nBar) != 0) {
        // Yes, the binary discards both calls' results.
        if (mPlaybackOn != 0 && mTutorial == 0) {
            mPlayMap->Slot16(nBar);
        } else {
            mPlayMap->Slot17(nBar);
        }
    }

    if (mGlobals->IsJukeboxMode() && nBar >= mEndBar && mEndState == kEndStateNone) {
        mEndState = kEndStateOver;
        mGlobals->GetWorld()->PostExitMode1();
    }

    if (mTutorial != 0 && mUnknown1c == 0) {
        Player *pPlayer = mPlayers[0];
        pPlayer->Slot2(); // Yes, the binary discards this call's result.
        pPlayer->AddJuice(kBarJuiceCost, 1);
    }
    const int nStreamBar = nBar - kSynthStreamLeadBars;
    if (mTutorial == 0 && nStreamBar >= 0) {
        SetSynthStreamBar(mPlayMap->Slot13(nStreamBar) + 1);
    }

    ScheduleBar(nBar + 1);
}

void Gamer::Start() {
    ScheduleBar(0);
}

void Gamer::SetBackGraphs(std::vector<BGTrackGraph *> *pGraphs) {
    mBackGraphs = pGraphs;
    mBackEnableMgr = GameEnableMgr::CreateReleasing(
        kBackTrackConfigCode, static_cast<int>(pGraphs->size()), this);
}

void Gamer::EnablePlayerFreestyle(int nStartBar, int nEndBar) {
    if (mTutorial == 0) {
        Fatal(kFreestyleOutsideTutorial);
    }
    mPlayers[0]->Slot8(nStartBar, nEndBar);
}

void Gamer::AddJuice(int nAmount) {
    mPlayers[0]->AddJuice(nAmount, 1);
}

void Gamer::EndWithScore(int nScore) {
    mEndBar = 0;
    if (nScore != 0) {
        mPlayers[0]->SetScore(nScore, kEndScoreCeiling);
    }
}

void Gamer::ScheduleBar(int nBar) {
    Mid::MBT when(std::min(std::max(mBarLength.mTick * nBar, kMBTMinimum), kMBTMaximum));
    if (when.mTick != Mid::MBT(0).mTick) {
        when.mTick = std::min(std::max(when.mTick - Mid::MBT(1).mTick, kMBTMinimum), kMBTMaximum);
    }

    GamerCmd *pCommand = new GamerCmd(this, nBar);
    mGlobals->GetSongClock()->PostAtSongTick(pCommand, when.mTick, mCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}
