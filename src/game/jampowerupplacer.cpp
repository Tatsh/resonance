#include "game/jampowerupplacer.h"

#include "app/application.h"
#include "game/powerupcollectioni.h"
#include "mid/tick.h"
#include "sch/tickclock.h"

namespace {

// MIDI ticks in one bar, the divisor DeployPowerup() applies to the song position.
constexpr int kTicksPerBar = 1920;

} // namespace

JamPowerupPlacer::JamPowerupPlacer(LocalPlayer *pOwner, PowerupCollectionI *pCollection)
    : mOwner(pOwner), mCollection(pCollection) {
}

JamPowerupPlacer::~JamPowerupPlacer() {
    // Everything left in the body is the expansion of the MsgSource destructor.
}

void JamPowerupPlacer::DeployPowerup() {
    const int nBar =
        Application::shared()->GetSongClock()->SongTick() / Sch::Tick(kTicksPerBar).mTick;
    mCollection->Deploy(mOwner->GetTrack(), nBar);
}
