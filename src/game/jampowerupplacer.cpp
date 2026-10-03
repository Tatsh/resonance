#include "game/jampowerupplacer.h"

#include "app/application.h"
#include "game/powerupcollectioni.h"
#include "mid/tick.h"
#include "sch/tickclock.h"

namespace {

// MIDI ticks in one bar, the divisor DeployPowerup() applies to the song position.
constexpr int kTicksPerBar = 1920;

} // namespace

// NTSC-U/C: 0x001cdf88, PAL: 0x001d3e40
JamPowerupPlacer::JamPowerupPlacer(LocalPlayer *pOwner, PowerupCollectionI *pCollection)
    : mOwner(pOwner), mCollection(pCollection) {
}

// NTSC-U/C: 0x001cdc58, PAL: 0x001d3b10
// Everything left in the body is the expansion of the MsgSource destructor.
JamPowerupPlacer::~JamPowerupPlacer() {
}

// NTSC-U/C: 0x001cdfe0, PAL: 0x001d3e98
void JamPowerupPlacer::DeployPowerup() {
    const int nBar =
        Application::shared()->GetSongClock()->SongTick() / Sch::Tick(kTicksPerBar).mTick;
    mCollection->Deploy(mOwner->GetTrack(), nBar);
}
