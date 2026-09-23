#include "app/playsound.h"

namespace {

// The sound numbers of the powerups that have one.
constexpr int kSoundAutocatcher = 0x3c;
constexpr int kSoundCrippler = 0x3d;
constexpr int kSoundFreestyler = 0x3e;
constexpr int kSoundNeutralizer = 0x3f;
constexpr int kSoundBumper = 0x40;
constexpr int kSoundMultiplier = 0x41;

constexpr int kPowerupSoundUnknown = -1;
constexpr int kPowerupSoundVelocity = 127;

} // namespace

// 0x0012f520
void PlayPowerupSound(HudItemKind kind) {
    int nSound;
    switch (kind) {
    case kHudItemNeutralizer:
        nSound = kSoundNeutralizer;
        break;
    case kHudItemCrippler:
        nSound = kSoundCrippler;
        break;
    case kHudItemFreestyler:
        nSound = kSoundFreestyler;
        break;
    case kHudItemAutocatcher:
        nSound = kSoundAutocatcher;
        break;
    case kHudItemBumper:
        nSound = kSoundBumper;
        break;
    case kHudItemMultiplier:
        nSound = kSoundMultiplier;
        break;
    default:
        return;
    }
    PlaySynthSound(nSound, kPowerupSoundUnknown, kPowerupSoundVelocity, 0);
}
