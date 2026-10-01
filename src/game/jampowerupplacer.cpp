#include "game/jampowerupplacer.h"

#include "app/application.h"
#include "game/powerupcollectioni.h"
#include "mid/mbt.h"
#include "sch/tickclock.h"

namespace {

// MIDI ticks in one bar, the divisor DeployPowerup() applies to the song position.
constexpr int kTicksPerBar = 1920;

} // namespace

// 0x001cdf88
JamPowerupPlacer::JamPowerupPlacer(LocalPlayer *pOwner, PowerupCollectionI *pCollection)
    : mOwner(pOwner), mCollection(pCollection) {
}

// 0x001cdc58
// Everything left in the body is the expansion of the MsgSource destructor.
JamPowerupPlacer::~JamPowerupPlacer() {
}

// 0x001cdfe0
void JamPowerupPlacer::DeployPowerup() {
    const int nBar =
        Application::shared()->GetSongClock()->SongTick() / Mid::MBT(kTicksPerBar).mTick;
    mCollection->Deploy(mOwner->GetTrack(), nBar);
}
