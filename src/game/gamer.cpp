#include "game/gamer.h"

#include <algorithm>

#include "app/application.h"
#include "app/playsound.h"
#include "game/bgtrackgraph.h"
#include "game/enablemgr.h"
#include "game/forcefeedbackmgr.h"
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
#include "msg/advancesectionmsg.h"
#include "msg/advancesectiontogglemsg.h"
#include "msg/cripplemsg.h"
#include "msg/cripplepacket.h"
#include "msg/enablefreestylemsg.h"
#include "msg/freestylefxmsg.h"
#include "msg/invalidateseekermsg.h"
#include "msg/invalidatetrackmsg.h"
#include "msg/phrasecapturedmsg.h"
#include "msg/playbackmodemsg.h"
#include "msg/playbacktogglemsg.h"
#include "msg/tracksonmsg.h"
#include "msg/winmsg.h"
#include "os/log.h"
#include "sch/tickclock.h"
#include "script/configquery.h"
#include "script/scripthost.h"
#include "synth/midi_main.h"

namespace {

// Configuration code of the solo game's track requirement lists.
constexpr int kSoloRequirementsConfigCode = 903;

// Configuration codes of the constructor's recorded values.
constexpr int kInvalidateBarsConfigCode = 702;
constexpr int kSoloJuiceConfigCode = 908;
constexpr int kSoloMaxJuiceConfigCode = 916;
constexpr int kTutorialConfigCode = 929;

// MIDI ticks in one bar.
constexpr int kTicksPerBar = 1920;

// The values the constructor starts its words at.
constexpr int kInitialUnreadSetting = 2;
constexpr int kInitialFreeEndBar = -1;
constexpr int kUnallocatedCommand = -2;

// The script template AdvanceTo() runs.
constexpr int kAdvanceScriptTemplate = 1013;

// The sounds a game's end plays.
constexpr char kWinSound[] = "SND_WIN";
constexpr char kLoseSound[] = "SND_LOSE";

// Bars after the last one at which a finished multiplayer game exits.
constexpr int kExitDelayBars = 4;

// The span a solo win passes to Player::SetFreestyleSpan().
constexpr int kWonBarSpan = 100000;

// ForceEnabled()'s end bar that frees a track for good.
constexpr int kFreeForever = -1;

// A solo player below this juice cannot continue.
constexpr int kMinimumJuice = 2;

// The juice a bar costs.
constexpr int kBarJuiceCost = -1;

// Bars the streamed audio runs behind the update.
constexpr int kSynthStreamLeadBars = 3;

// Configuration code of the background tracks' enable policy.
constexpr int kBackTrackConfigCode = 902;

// The length of the freestyle span OnEnableFreestyle() grants.
constexpr int kFreestyleBars = 8;

// The victims OnCripple() reserves room for.
constexpr unsigned kCrippleVictimCapacity = 3;

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

// NTSC-U/C: 0x00110138, PAL: 0x00110598
Gamer::Gamer(int nTrackCount, int nEndBar, GameStats *pStats)
    : mJuiceFrozen(0), mEndState(kEndStateNone), mJukeboxMode(0), mCurrentBar(0),
      mTrackCount(nTrackCount), mUnreadSetting(kInitialUnreadSetting), mUnreadFlag(0),
      mPlaybackOn(0), mGlobals(Application::shared()), mStats(pStats), mBarLength(kTicksPerBar),
      mPlayers(mGlobals->GetWorld()->mPlayers), mCommand{kUnallocatedCommand}, mBackGraphs(nullptr),
      mGraphs(nullptr), mTrackSources(nTrackCount, MsgSource()), mFreestyleEndBar(0),
      mEnableMgr(nullptr), mBackEnableMgr(nullptr), mCheated(0) {
    mFreeEndBar = kInitialFreeEndBar;
    mPlayMap = mGlobals->GetPlayMap();
    mEndBar = nEndBar;
    mInvalidateBars = QueryConfigValue(kInvalidateBarsConfigCode);
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

// NTSC-U/C: 0x00110930, PAL: 0x00110d90
Gamer::~Gamer() {
    Withdraw();
    delete mEnableMgr;
    delete mBackEnableMgr;
}

// NTSC-U/C: 0x00116c40, PAL: 0x001170f8
void Gamer::Withdraw() {
    const CmdID command = mCommand;
    mGlobals->GetSongClock()->Withdraw(command);
}

// NTSC-U/C: 0x00112978, PAL: 0x00112dd8
void Gamer::DispatchPriv(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == AdvanceSectionMsg::sID) {
        OnMsg(*static_cast<AdvanceSectionMsg *>(pMsg));
    } else if (nType == g_nPhraseCapturedMsgType) {
        OnPhraseCaptured(static_cast<PhraseCapturedMsg *>(pMsg));
    } else if (nType == g_nEnableFreestyleMsgType) {
        OnEnableFreestyle(static_cast<EnableFreestyleMsg *>(pMsg));
    } else if (nType == PlaybackModeMsg::sID) {
        OnPlaybackMode(static_cast<PlaybackModeMsg *>(pMsg));
    } else if (nType == CrippleMsg::sID) {
        OnCripple(static_cast<CrippleMsg *>(pMsg));
    }
}

// NTSC-U/C: 0x00116920, PAL: 0x00116dd8
void Gamer::OnMsg(const AdvanceSectionMsg &msg) {
    const bool bJamAdvance = mPlayMode == kPlayModeJam && mGameMode != kGameModeNet;
    if (!bJamAdvance && mTutorial == 0) {
        return;
    }
    if (msg.mPlayer->GetInputSlot() != 0) {
        return;
    }
    AdvanceAt(msg.mPosition);
}

// NTSC-U/C: 0x001169a8, PAL: 0x00116e60
void Gamer::OnPhraseCaptured(PhraseCapturedMsg *pMsg) {
    if (mPlayMode == kPlayModeJam || mEndState != kEndStateNone) {
        return;
    }
    pMsg->mPlayer->Dispatch(pMsg);
    if (mTutorial == 0 && mGameMode == kGameModeSolo) {
        FreeTracksAfterCapture(pMsg->mFirstBar); // Yes, the binary discards this call's result.
    }
}

// NTSC-U/C: 0x00111080, PAL: 0x001114e0
void Gamer::OnEnableFreestyle(EnableFreestyleMsg *pMsg) {
    const int nTick = mGlobals->GetSongClock()->SongTick();
    Player *pPlayer = pMsg->mPlayer;
    const int nTrack = pPlayer->GetTrack();
    if (GetTrack(nTrack)->mKind == kTrackModeCatch) {
        return;
    }

    const int nBar = pMsg->mBar;
    const int nEndBar = nBar + kFreestyleBars;
    pPlayer->SetFreestyleSpan(nBar, nEndBar);
    mEnableMgr->ForceEnabled(nTrack, nBar, nEndBar);

    InvalidateSeekerMsg invalidateSeeker(nTick / mBarLength.mTick, nTrack);
    mTrackSources[nTrack].Send(&invalidateSeeker);
    mFreestyleEndBar = nEndBar;
    pMsg->mResult = 1;

    FreestyleFXMsg freestyle(nTrack, nBar, nEndBar);
    Send(&freestyle);
}

// NTSC-U/C: 0x00110e60, PAL: 0x001112c0
void Gamer::OnPlaybackMode(PlaybackModeMsg *pMsg) {
    if (mPlayMode != kPlayModeJam) {
        return;
    }
    if (pMsg->mPlayer->GetInputSlot() != 0) {
        return;
    }

    const int nBar = std::max(pMsg->mPosition.mTick / mBarLength.mTick, 0);
    InputMap *pInputMap = InputMap::shared();
    mPlaybackOn ^= 1;
    if (mPlaybackOn != 0) {
        pInputMap->StopAllRiffs();
        pInputMap->DisableEntries();
        pInputMap->SetEnabled(kJukeboxSlot, InputMap::kActionPlayback, 1);
        pInputMap->SetEnabled(kJukeboxSlot, InputMap::kActionRotateLeft, 1);
        pInputMap->SetEnabled(kJukeboxSlot, InputMap::kActionRotateRight, 1);
        mSectionRepeats = mPlayMap->IsLooping(nBar);
        mPlayMap->EndLoop(nBar); // Yes, the binary discards this call's result.
    } else {
        pInputMap->EnableEntries();
        mSectionRepeats = 1;
        mPlayMap->StartLoop(nBar); // Yes, the binary discards this call's result.
    }

    ForceFeedbackMgr *pForceFeedback = mGlobals->GetWorld()->mForceFeedback;
    pForceFeedback->SetJukeboxMode(mPlaybackOn);
    pForceFeedback->StartMetronome(Mid::MBT(0));

    PlaybackToggleMsg toggle(mPlaybackOn);
    Send(&toggle);
    AdvanceTo(nBar, mPlayMap->IsLooping(nBar) ^ 1);
}

// NTSC-U/C: 0x00111230, PAL: 0x00111690
void Gamer::OnCripple(CrippleMsg *pMsg) {
    std::vector<Player *> victims;
    victims.reserve(kCrippleVictimCapacity);

    bool bFound = false;
    Player *pAttacker = pMsg->mPlayer;
    const int nTrack = pMsg->mTrack;
    for (auto it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        if (*it != pAttacker && (*it)->GetTrack() == nTrack) {
            bFound = true;
            victims.push_back(*it);
        }
    }

    if (bFound) {
        pMsg->mResult = 1;
        CripplePacket packet(pAttacker, victims);
        Send(&packet);
    }
}

// NTSC-U/C: 0x00110ba0, PAL: 0x00111000
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
            if (IsFreestyleTrack(i)) {
                mEnableMgr->ForceDisable(i);
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

// NTSC-U/C: 0x00116828, PAL: 0x00116ce0
void Gamer::SetBarOwner(int nTrack, int nBar, Player *pPlayer) {
    mEnableMgr->SetBarOwner(nTrack, nBar, pPlayer);
}

// NTSC-U/C: 0x00116858, PAL: 0x00116d10
int Gamer::QueryBar(int nTrack, int nBar) {
    return mEnableMgr->QueryBar(nTrack, nBar);
}

// NTSC-U/C: 0x00116888, PAL: 0x00116d40
bool Gamer::IsFreestyleTrack(int nTrack) {
    return GetTrack(nTrack)->mKind != kTrackModeCatch;
}

// NTSC-U/C: 0x001168b0, PAL: 0x00116d68
PhraseDatabase *Gamer::GetPhraseDatabase(int nTrack) {
    return (*mGraphs)[nTrack]->GetPhraseDatabase();
}

// NTSC-U/C: 0x001168e0, PAL: 0x00116d98
TrackData *Gamer::GetTrack(int nTrack) {
    return mGlobals->GetLevel()->TrackAt(nTrack);
}

// NTSC-U/C: 0x001116c8, PAL: 0x00111b28
void Gamer::AdvanceTo(int nBar, int nAdvance) {
    const Mid::MBT position(
        std::min(std::max(nBar * Mid::MBT(kTicksPerBar).mTick, kMBTMinimum), kMBTMaximum));
    AdvanceSectionToggleMsg toggle(nAdvance, position);
    Send(&toggle);

    const int nStart = mPlayMap->MapBar(mPlayMap->FollowingStepBar(nBar));
    const int nEnd = nStart + mInvalidateBars;
    for (int i = 0; i < mTrackCount; ++i) {
        InvalidateTrackMsg invalidateTrack(nStart, nEnd, i);
        mTrackSources[i].Send(&invalidateTrack);
        InvalidateSeekerMsg invalidateSeeker(nBar, i);
        mTrackSources[i].Send(&invalidateSeeker);
    }

    CallScriptTemplate(kAdvanceScriptTemplate);
}

// NTSC-U/C: 0x00116a30, PAL: 0x00116ee8
void Gamer::AdvanceAt(Mid::MBT position) {
    const int nBar = position.mTick / mBarLength.mTick;
    AdvanceTo(nBar, mPlayMap->ToggleLoop(nBar));
}

// NTSC-U/C: 0x00111c90, PAL: 0x001120f0
bool Gamer::SendTracksOn(int nBar) {
    int nOwnedTracks = 0;
    int nOpenTracks = 0;
    const int nStep = mPlayMap->MapBar(nBar);
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

// NTSC-U/C: 0x00111e30, PAL: 0x00112290
bool Gamer::FreeTracksAfterCapture(int nBar) {
    const bool bComplete = SendTracksOn(nBar);
    if (!bComplete) {
        return bComplete;
    }

    const int nNextBar = mPlayMap->FollowingStepBar(nBar);
    mPlayers[0]->SetFreestyleSpan(nBar, nNextBar);
    mFreestyleEndBar = nBar + 1;

    int nFreeEndBar = mFreeEndBar;
    for (int i = 0; i < mTrackCount; ++i) {
        if (GetTrack(i)->mKind == kTrackModeCatch || mFreeEndBar >= nNextBar) {
            continue;
        }

        const int nStartBar = std::max(nBar, mFreeEndBar);
        mEnableMgr->ForceEnabled(i, nStartBar, nNextBar);
        FreestyleFXMsg msg(i, nStartBar, nNextBar);
        Send(&msg);
        nFreeEndBar = nNextBar;
    }
    mFreeEndBar = nFreeEndBar;
    return bComplete;
}

// NTSC-U/C: 0x001118c0, PAL: 0x00111d20
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

// NTSC-U/C: 0x00111b78, PAL: 0x00111fd8
void Gamer::RecordSoloStats(int bCompleted, int nBar) {
    mStats->mCompleted = bCompleted;
    mStats->mCheated = mCheated;
    mStats->SetScore(0, mPlayers[0]->GetScore());
    if (bCompleted != 0) {
        mStats->SetProgress(kCompleteProgress);
    } else {
        mStats->SetProgress(static_cast<float>(nBar) / static_cast<float>(mPlayMap->GetEndBar()));
    }
    mStats->SetTally(0, mPlayers[0]->GetBestStreak());
    mStats->SetRatio(0, mPlayers[0]->GetCaptureRatio());
}

// NTSC-U/C: 0x00111fa8, PAL: 0x00112408
void Gamer::OnBar(int nBar) {
    mPlayMap->MapBar(nBar); // Yes, the binary discards this call's result.
    mCurrentBar = nBar;
    for (unsigned i = 0; i < mBackGraphs->size(); ++i) {
        if (mBackEnableMgr->QueryBar(i, nBar) != 0) {
            (*mBackGraphs)[i]->EnableMidi();
        } else {
            (*mBackGraphs)[i]->DisableMidi();
        }
    }

    if (mPlayMode == kPlayModeGame && mTutorial == 0) {
        if (mGameMode != kGameModeSolo) {
            if (nBar == mEndBar && mEndState == kEndStateNone) {
                DeclareWinners();
            }
            if (nBar == mEndBar + kExitDelayBars && mEndState == kEndStateOver) {
                mGlobals->GetWorld()->PostFinish();
            }
        } else {
            Player *pPlayer = mPlayers[0];
            pPlayer->GetInputSlot(); // Yes, the binary discards this call's result.
            if (nBar >= mEndBar && mEndState == kEndStateNone) {
                mEndState = kEndStateWon;
                pPlayer->SetFreestyleSpan(nBar, nBar + kWonBarSpan);

                {
                    WinMsg win;
                    win.AddWinner(pPlayer);
                    Send(&win);
                }
                PlaySoundByName(kWinSound);
                RecordSoloStats(1, nBar);

                {
                    TracksOnMsg tracksOn(nBar, 0);
                    Send(&tracksOn);
                }
                for (int i = 0; i < mTrackCount; ++i) {
                    if (GetTrack(i)->mKind != kTrackModeCatch) {
                        mEnableMgr->ForceEnabled(i, 0, kFreeForever);
                    } else if ((*mGraphs)[i]->CanGivePhrases() != 0) {
                        (*mGraphs)[i]->GivePhrases(nBar, pPlayer);
                    }
                }
            } else if (pPlayer->GetJuice() < kMinimumJuice) {
                if (mEndState != kEndStateNone) {
                    InputMap::shared()->EnableEntries();
                    mGlobals->GetWorld()->PostFinish();
                } else {
                    bool bExhausted = true;
                    for (int i = 0; i < mTrackCount; ++i) {
                        if ((*mGraphs)[i]->HasNothingPending() == 0) {
                            bExhausted = false;
                            break;
                        }
                    }

                    if (bExhausted) {
                        {
                            WinMsg lose;
                            Send(&lose);
                        }
                        mEndState = kEndStateOver;
                        if (mJuiceFrozen == 0) {
                            pPlayer->AddJuice(kBarJuiceCost, 1);
                        }
                        PlaySoundByName(kLoseSound);
                        RecordSoloStats(0, nBar);
                        InputMap::shared()->DisableEntries();
                    }
                }
            } else if (mEndState == kEndStateNone && !FreeTracksAfterCapture(nBar) &&
                       mJuiceFrozen == 0) {
                pPlayer->AddJuice(kBarJuiceCost, 1);
            }
        }
    }

    if ((mPlayMode == kPlayModeJam || mTutorial != 0) && mPlayMap->IsStepStart(nBar) != 0) {
        // Yes, the binary discards both calls' results.
        if (mPlaybackOn != 0 && mTutorial == 0) {
            mPlayMap->EndLoop(nBar);
        } else {
            mPlayMap->StartLoop(nBar);
        }
    }

    if (mGlobals->IsJukeboxMode() && nBar >= mEndBar && mEndState == kEndStateNone) {
        mEndState = kEndStateOver;
        mGlobals->GetWorld()->PostFinish();
    }

    if (mTutorial != 0 && mJuiceFrozen == 0) {
        Player *pPlayer = mPlayers[0];
        pPlayer->GetInputSlot(); // Yes, the binary discards this call's result.
        pPlayer->AddJuice(kBarJuiceCost, 1);
    }
    const int nStreamBar = nBar - kSynthStreamLeadBars;
    if (mTutorial == 0 && nStreamBar >= 0) {
        SetSynthStreamBar(mPlayMap->GetAbsoluteSectionIndex(nStreamBar) + 1);
    }

    ScheduleBar(nBar + 1);
}

// NTSC-U/C: 0x00116c20, PAL: 0x001170d8
void Gamer::Start() {
    ScheduleBar(0);
}

// NTSC-U/C: 0x001167e0, PAL: 0x00116c98
void Gamer::SetBackGraphs(std::vector<BGTrackGraph *> *pGraphs) {
    mBackGraphs = pGraphs;
    mBackEnableMgr = GameEnableMgr::CreateReleasing(
        kBackTrackConfigCode, static_cast<int>(pGraphs->size()), this);
}

// NTSC-U/C: 0x00116a98, PAL: 0x00116f50
void Gamer::EnablePlayerFreestyle(int nStartBar, int nEndBar) {
    if (mTutorial == 0) {
        Fatal(kFreestyleOutsideTutorial);
        return;
    }
    mPlayers[0]->SetFreestyleSpan(nStartBar, nEndBar);
}

// NTSC-U/C: 0x00116ae8, PAL: 0x00116fa0
void Gamer::AddJuice(int nAmount) {
    mPlayers[0]->AddJuice(nAmount, 1);
}

// NTSC-U/C: 0x00116b18, PAL: 0x00116fd0
void Gamer::EndWithScore(int nScore) {
    mEndBar = 0;
    if (nScore != 0) {
        mPlayers[0]->SetScore(nScore, kEndScoreCeiling);
    }
}

// NTSC-U/C: 0x00112838, PAL: 0x00112c98
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
