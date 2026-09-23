#include "game/gamer.h"

#include "app/application.h"
#include "game/enablemgr.h"
#include "game/gameenablemgr.h"
#include "game/gamemanagerimpl.h"
#include "game/grooveworld.h"
#include "game/leveldata.h"
#include "game/localjamenablemgr.h"
#include "game/netjamenablemgr.h"
#include "game/scoretrackgraph.h"
#include "game/trackdata.h"

namespace {

// Configuration code of the solo game's track requirement lists.
constexpr int kSoloRequirementsConfigCode = 0x387;

// The owner buckets a network jam lists as open.
constexpr int kNetJamBucketCount = 8;

// A network jam with at least this many players shares the tracks among them.
constexpr unsigned kSharedTrackPlayerCount = 2;

} // namespace

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
