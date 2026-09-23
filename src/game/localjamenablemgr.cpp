#include "game/localjamenablemgr.h"

#include <vector>

#include "app/application.h"
#include "game/leveldata.h"
#include "game/notefinder.h"
#include "game/trackdata.h"
#include "mid/mbt.h"
#include "mid/tickobj.h"
#include "msg/musemsg.h"

namespace {

// MIDI ticks in one bar. QueryBar() searches the previous bar from this position.
constexpr int kTicksPerBar = 1920;

} // namespace

LocalJamEnableMgr::LocalJamEnableMgr() {
    for (int i = 0; i < kTrackCount; ++i) {
        mTracks[i] = Application::shared()->GetLevel()->TrackAt(i);
    }
}

LocalJamEnableMgr *LocalJamEnableMgr::CreateSolo() {
    return new LocalJamEnableMgr;
}

LocalJamEnableMgr *LocalJamEnableMgr::CreateLocal() {
    return new LocalJamEnableMgr;
}

void LocalJamEnableMgr::SetBarOwner(int, int, Player *) {
}

int LocalJamEnableMgr::QueryBar(int nTrack, int nBar) {
    TrackData *pTrack = mTracks[nTrack];
    if (pTrack->mKind != kTrackModeVocal) {
        return 1;
    }

    NoteFinder finder;
    if (nBar > 0) {
        finder.Search(pTrack->GetMidi(nBar - 1), Mid::MBT(kTicksPerBar));
    }
    finder.Search(pTrack->GetMidi(nBar), Mid::MBT(0));
    return finder.mFound;
}
