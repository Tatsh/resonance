#include "game/gamer.h"

#include "app/application.h"
#include "game/enablemgr.h"
#include "game/gameenablemgr.h"
#include "game/gamemanagerimpl.h"
#include "game/grooveworld.h"
#include "game/inputmap.h"
#include "game/leveldata.h"
#include "game/localjamenablemgr.h"
#include "game/netjamenablemgr.h"
#include "game/player.h"
#include "game/scoretrackgraph.h"
#include "game/trackdata.h"
#include "sch/tickclock.h"
#include "script/configquery.h"

namespace {

// Configuration code of the solo game's track requirement lists.
constexpr int kSoloRequirementsConfigCode = 0x387;

// Configuration codes of the constructor's recorded values.
constexpr int kUnknown3cConfigCode = 0x2be;
constexpr int kSoloJuiceConfigCode = 0x38c;
constexpr int kSoloMaxJuiceConfigCode = 0x394;
constexpr int kDisplayModeConfigCode = 0x3a1;

// MIDI ticks in one bar.
constexpr int kTicksPerBar = 1920;

// The values the constructor starts its words at.
constexpr int kInitialUnknown44 = 2;
constexpr int kInitialUnknown88 = -1;
constexpr int kUnallocatedCommand = -2;

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

Gamer::Gamer(int nTrackCount, int nUnknown24, GameStats *pStats)
    : mUnknown1c(0), mUnknown28(0), mJukeboxMode(0), mUnknown38(0), mTrackCount(nTrackCount),
      mUnknown44(kInitialUnknown44), mUnknown48(0), mPlaybackOn(0), mGlobals(Application::shared()),
      mStats(pStats), mBarLength(kTicksPerBar), mPlayers(mGlobals->GetWorld()->mPlayers),
      mBackGraphs(nullptr), mGraphs(nullptr), mTrackSources(nTrackCount, MsgSource()),
      mUnknown84(0), mEnableMgr(nullptr), mUnknown94(nullptr), mUnknown98(0) {
    mCommand.mValue = kUnallocatedCommand;
    mUnknown88 = kInitialUnknown88;
    mPlayMap = mGlobals->GetPlayMap();
    mUnknown24 = nUnknown24;
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
    mUnknown18 = QueryConfigFlag(kDisplayModeConfigCode);
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
    delete mUnknown94;
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
