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

// 0x001cde60
SimplifiedGamePowerupPlacer::SimplifiedGamePowerupPlacer(LocalPlayer *pOwner,
                                                         PowerupCollectionI *pCollection)
    : mOwner(pOwner), mCollection(pCollection) {
}

// 0x001cdb20
SimplifiedGamePowerupPlacer::~SimplifiedGamePowerupPlacer() {
}

// 0x001cdeb8
void SimplifiedGamePowerupPlacer::OnUnknownSlot8() {
    const int nTick = Application::shared()->GetSongClock()->SongTick();
    const int nBar = nTick / Mid::MBT(kTicksPerBar).mTick;
    if (nBar < Application::shared()->GetPlayMap()->Slot9()) {
        mCollection->Deploy(mOwner->Slot4(), nBar);
    }
}
