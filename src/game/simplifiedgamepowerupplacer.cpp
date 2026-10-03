#include "game/simplifiedgamepowerupplacer.h"

#include "app/application.h"
#include "game/localplayer.h"
#include "game/playmap.h"
#include "game/powerupcollectioni.h"
#include "mid/mbt.h"
#include "sch/tickclock.h"

namespace {

// MIDI ticks in one bar.
constexpr int kTicksPerBar = 1920;

} // namespace

// NTSC-U/C: 0x001cde60, PAL: 0x001d3d18
SimplifiedGamePowerupPlacer::SimplifiedGamePowerupPlacer(LocalPlayer *pOwner,
                                                         PowerupCollectionI *pCollection)
    : mOwner(pOwner), mCollection(pCollection) {
}

// NTSC-U/C: 0x001cdb20, PAL: 0x001d39d8
SimplifiedGamePowerupPlacer::~SimplifiedGamePowerupPlacer() {
}

// NTSC-U/C: 0x001cdeb8, PAL: 0x001d3d70
void SimplifiedGamePowerupPlacer::DeployPowerup() {
    const int nTick = Application::shared()->GetSongClock()->SongTick();
    const int nBar = nTick / Mid::MBT(kTicksPerBar).mTick;
    if (nBar < Application::shared()->GetPlayMap()->GetEndBar()) {
        mCollection->Deploy(mOwner->GetTrack(), nBar);
    }
}
