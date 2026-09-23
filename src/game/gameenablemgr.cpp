#include "game/gameenablemgr.h"

#include "app/application.h"
#include "game/gamer.h"
#include "game/phrasedatabase.h"
#include "game/player.h"
#include "game/playmap.h"
#include "msg/invalidatetrackmsg.h"
#include "script/configquery.h"

namespace {

// The size of SetBarOwner()'s table of track states before a change. The stack frame reserves
// twelve words for it.
constexpr int kMaxTrackCount = 12;

// The free-until bar each track starts at.
constexpr int kNoFreeBar = -1;

// The first bar SetFreeUntil() invalidates.
constexpr int kFirstBar = 0;

// The arguments SetBarOwner() passes to SetFreeUntil() to release a track for good.
constexpr int kReleaseBar = 0;
constexpr int kReleaseUntilBar = -1;

} // namespace

// 0x001011c8
GameEnableMgr::GameEnableMgr(int nConfigCode, int nTrackCount, Gamer *pGamer, int nReleaseWhenMet)
    : mTrackCount(nTrackCount), mOwnedTrackCount(kOwnedTrackCount),
      mFreeUntil(nTrackCount, kNoFreeBar) {
    mGamer = pGamer;
    mPlayMap = Application::shared()->GetPlayMap();
    mReleaseWhenMet = nReleaseWhenMet;
    Init(nConfigCode);
}

// 0x00101588
GameEnableMgr::GameEnableMgr(int nTrackCount, Gamer *pGamer, int nReleaseWhenMet)
    : mTrackCount(nTrackCount), mOwnedTrackCount(kOwnedTrackCount),
      mFreeUntil(nTrackCount, kNoFreeBar) {
    mGamer = pGamer;
    mPlayMap = Application::shared()->GetPlayMap();
    mReleaseWhenMet = nReleaseWhenMet;
    mRequirements.clear();
    mRequirements.resize(mTrackCount, std::vector<int>());
}

// 0x00105280
GameEnableMgr *GameEnableMgr::CreateReleasing(int nConfigCode, int nTrackCount, Gamer *pGamer) {
    return new GameEnableMgr(nConfigCode, nTrackCount, pGamer, 1);
}

// 0x00105308
GameEnableMgr *GameEnableMgr::CreateInvalidating(int nConfigCode, int nTrackCount, Gamer *pGamer) {
    return new GameEnableMgr(nConfigCode, nTrackCount, pGamer, 0);
}

// 0x00105390
GameEnableMgr *GameEnableMgr::CreateUnrestricted(int nTrackCount, Gamer *pGamer) {
    return new GameEnableMgr(nTrackCount, pGamer, 0);
}

// 0x00101f10
void GameEnableMgr::Init(int nConfigCode) {
    mRequirements.clear();
    mRequirements.resize(mTrackCount, std::vector<int>());

    std::vector<int> values;
    for (int i = 0; i < mTrackCount; ++i) {
        values.clear();
        QueryConfigVector(&values, nConfigCode, i + 1);
        for (std::vector<int>::iterator it = values.begin(); it != values.end(); ++it) {
            mRequirements[i].push_back(*it - 1);
        }
    }
}

// 0x00105498
void GameEnableMgr::FindOwnedTracks(int *pOwned, int nBar) {
    for (int i = 0; i < mOwnedTrackCount; ++i) {
        pOwned[i] = mGamer->GetPhraseDatabase(i)->GetOwner(nBar)->IsNull() ^ 1;
    }
}

// 0x00105530
int GameEnableMgr::IsTrackEnabled(int nTrack, const int *pOwned) {
    const std::vector<int> &requirements = mRequirements[nTrack];
    for (std::vector<int>::const_iterator it = requirements.begin(); it != requirements.end();
         ++it) {
        if (*it == kNeverEnabled || pOwned[*it] == 0) {
            return 0;
        }
    }
    return 1;
}

// 0x00101d70
void GameEnableMgr::SetBarOwner(int nTrack, int nBar, Player *pPlayer) {
    int owned[kOwnedTrackCount];
    FindOwnedTracks(owned, nBar);

    int enabled[kMaxTrackCount];
    for (int i = 0; i < mTrackCount; ++i) {
        enabled[i] = IsTrackEnabled(i, owned);
    }
    owned[nTrack] = pPlayer->IsNull() ^ 1;

    for (int i = 0; i < mTrackCount; ++i) {
        if (enabled[i] == IsTrackEnabled(i, owned)) {
            continue;
        }

        if (mReleaseWhenMet != 0) {
            SetFreeUntil(i, kReleaseBar, kReleaseUntilBar);
        } else {
            InvalidateTrackMsg msg(nBar, nBar + 1, i);
            mGamer->mTrackSources[i].Send(&msg);
        }
    }
}

// 0x00101c68
void GameEnableMgr::SetFreeUntil(int nTrack, int nBar, int nUntilBar) {
    if (nUntilBar < nBar) {
        mRequirements[nTrack].clear();
    } else {
        mFreeUntil[nTrack] = nUntilBar;
    }

    InvalidateTrackMsg msg(kFirstBar, mPlayMap->mSteps.back(), nTrack);
    mGamer->mTrackSources[nTrack].Send(&msg);
}

// 0x00101bb0
void GameEnableMgr::DisableTrack(int nTrack) {
    mRequirements[nTrack].clear();
    mRequirements[nTrack].push_back(kNeverEnabled);
}

// 0x00105408
int GameEnableMgr::QueryBar(int nTrack, int nBar) {
    if (nBar < mFreeUntil[nTrack]) {
        return 1;
    }

    int owned[kOwnedTrackCount];
    FindOwnedTracks(owned, mPlayMap->Slot5(nBar));
    return IsTrackEnabled(nTrack, owned);
}
