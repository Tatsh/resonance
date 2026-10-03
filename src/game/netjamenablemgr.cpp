#include "game/netjamenablemgr.h"

#include <algorithm>

#include "app/application.h"
#include "game/gamer.h"
#include "game/grooveworld.h"
#include "game/phrasedatabase.h"
#include "game/player.h"
#include "game/playmap.h"
#include "msg/invalidatetrackmsg.h"

namespace {

// The size of SetBarOwner()'s table of track states before a change. The stack frame reserves
// twelve words for it.
constexpr int kMaxTrackCount = 12;

} // namespace

// NTSC-U/C: 0x00102790, PAL: 0x00102790
NetJamEnableMgr::NetJamEnableMgr(int nTrackCount,
                                 int nMaxOwned,
                                 const std::vector<int> *pOpenTracks,
                                 Gamer *pGamer)
    : mPlayMap(Application::shared()->GetPlayMap()), mGamer(pGamer),
      mLocalId(Application::shared()->GetWorld()->mLocalPlayers[0]->mPlayerId),
      mTrackCount(nTrackCount), mMaxOwned(nMaxOwned), mOpenTracks(*pOpenTracks),
      mSteps(&Application::shared()->GetPlayMap()->mSteps) {
    const int nSectionCount = mSteps->size() - 1;
    mOwners =
        std::vector<std::vector<int> >(nSectionCount, std::vector<int>(mTrackCount, kNoOwner));
}

// NTSC-U/C: 0x00105958, PAL: 0x00105958
NetJamEnableMgr *NetJamEnableMgr::Create(int nTrackCount,
                                         int nMaxOwned,
                                         const std::vector<int> *pOpenTracks,
                                         Gamer *pGamer) {
    return new NetJamEnableMgr(nTrackCount, nMaxOwned, pOpenTracks, pGamer);
}

// NTSC-U/C: 0x00105b08, PAL: 0x00105b08
int NetJamEnableMgr::FindSection(int nBar) {
    return std::upper_bound(mSteps->begin(), mSteps->end(), nBar) - 1 - mSteps->begin();
}

// NTSC-U/C: 0x00105a98, PAL: 0x00105a98
int NetJamEnableMgr::IsSongSectionEnabled(int nTrack, int nSection) const {
    const std::vector<int> &owners = mOwners[nSection];
    const int nOwner = owners[nTrack];
    if (nOwner == mLocalId) {
        return 1;
    }
    if (nOwner != kNoOwner) {
        return 0;
    }
    return std::count(owners.begin(), owners.end(), mLocalId) < mMaxOwned;
}

// NTSC-U/C: 0x00103158, PAL: 0x00103158
void NetJamEnableMgr::SetBarOwner(int nTrack, int nBar, Player *) {
    if (mOpenTracks.size() < static_cast<unsigned>(mTrackCount)) {
        return;
    }

    const int nSection = FindSection(nBar);
    int &owner = mOwners[nSection][nTrack];
    const int nOldOwner = owner;
    const int nNewOwner = mGamer->GetPhraseDatabase(nTrack)->GetOwner(nBar)->mPlayerId;
    if (nOldOwner == nNewOwner) {
        return;
    }

    int available[kMaxTrackCount];
    for (int i = 0; i < mTrackCount; ++i) {
        available[i] = IsSongSectionEnabled(i, nSection);
    }
    owner = nNewOwner;

    for (int i = 0; i < mTrackCount; ++i) {
        if (available[i] == IsSongSectionEnabled(i, nSection)) {
            continue;
        }

        InvalidateTrackMsg msg((*mSteps)[nSection], (*mSteps)[nSection + 1], i);
        mGamer->mTrackSources[i].Send(&msg);
    }
}

// NTSC-U/C: 0x001059f0, PAL: 0x001059f0
int NetJamEnableMgr::QueryBar(int nTrack, int nBar) {
    if (mOpenTracks.size() < static_cast<unsigned>(mTrackCount)) {
        return std::find(mOpenTracks.begin(), mOpenTracks.end(), nTrack) != mOpenTracks.end();
    }
    return IsSongSectionEnabled(nTrack, FindSection(mPlayMap->MapBar(nBar)));
}
