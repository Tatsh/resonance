#include "game/simplifiedgamepowerupplacer.h"

#include "app/application.h"
#include "game/localplayer.h"
#include "game/playmap.h"
#include "game/powerupcollectioni.h"
#include "mid/tick.h"
#include "sch/tickclock.h"

namespace {

// MIDI ticks in one bar.
constexpr int kTicksPerBar = 1920;

} // namespace

SimplifiedGamePowerupPlacer::SimplifiedGamePowerupPlacer(LocalPlayer *pOwner,
                                                         PowerupCollectionI *pCollection)
    : mOwner(pOwner), mCollection(pCollection) {
}

SimplifiedGamePowerupPlacer::~SimplifiedGamePowerupPlacer() {
}

void SimplifiedGamePowerupPlacer::DeployPowerup() {
    const int nTick = Application::shared()->GetSongClock()->SongTick();
    const int nBar = nTick / Sch::Tick(kTicksPerBar).mTick;
    if (nBar < Application::shared()->GetPlayMap()->GetEndBar()) {
        mCollection->Deploy(mOwner->GetTrack(), nBar);
    }
}
