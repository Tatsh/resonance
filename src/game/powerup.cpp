#include "game/powerup.h"

#include "app/hudutil.h"
#include "game/autocatchpowerup.h"
#include "game/bumppowerup.h"
#include "game/cripplepowerup.h"
#include "game/effectpowerup.h"
#include "game/freestylepowerup.h"
#include "game/ghostnotespowerup.h"
#include "game/multiplierpowerup.h"
#include "game/neutralizepowerup.h"

// 0x001ca4c8
Powerup::~Powerup() {
}

// 0x001c65f0
Powerup *Powerup::CreateForType(int nType) {
    switch (nType) {
    case kHudItemNeutralizer:
        return new NeutralizePowerup;
    case kHudItemCrippler:
        return new CripplePowerup;
    case kHudItemFreestyler:
        return new FreestylePowerup;
    case kHudItemAutocatcher:
        return new AutocatchPowerup;
    case kHudItemBumper:
        return new BumpPowerup;
    case kHudItemVolume:
    case kHudItemWah:
    case kHudItemStutter:
    case kHudItemEcho:
    case kHudItemFlange:
    case kHudItemChorus:
        return new EffectPowerup(nType);
    case kHudItemGuides:
        return new GhostNotesPowerup;
    case kHudItemMultiplier:
        return new MultiplierPowerup;
    default:
        return nullptr;
    }
}
