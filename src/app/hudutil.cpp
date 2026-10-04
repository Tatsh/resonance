#include "app/hudutil.h"

#include "met/metstrings.h"
#include "os/formatstring.h"

namespace {

// Share of full red in the colour `purple` selects.
constexpr float kPurpleRed = 0.65f;

} // namespace

// NTSC-U/C: 0x006dfde8, PAL: 0x00723618
int g_nHudNameCounter;

HxStr HudPowerupName(int nKind) {
    switch (nKind) {
    case kHudItemNeutralizer:
        return MetText(kMetStrIngNeutralizer, "NEUTRALIZER");
    case kHudItemCrippler:
        return MetText(kMetStrIngCrippler, "CRIPPLER");
    case kHudItemFreestyler:
        return MetText(kMetStrIngFreestyler, "FREESTYLER");
    case kHudItemAutocatcher:
        return MetText(kMetStrIngAutocatcher, "AUTOCATCHER");
    case kHudItemBumper:
        return MetText(kMetStrIngBumper, "BUMPER");
    case kHudItemVolume:
        return MetText(kMetStrIngVolume, "Volume");
    case kHudItemWah:
        return MetText(kMetStrIngWah, "Wah");
    case kHudItemStutter:
        return MetText(kMetStrIngStutter, "Stutter");
    case kHudItemEcho:
        return MetText(kMetStrIngEcho, "Echo");
    case kHudItemFlange:
        return MetText(kMetStrIngFlange, "Flange");
    case kHudItemChorus:
        return MetText(kMetStrIngChorus, "Chorus");
    case kHudItemGuides:
        return MetText(kMetStrIngGuides, "Guides");
    case kHudItemMultiplier:
        return MetText(kMetStrIngMultiplier, "MULTIPLIER");
    default:
        return HxStr("");
    }
}

Color HudColorFromName(HxStr name) {
    if (name == "green") {
        return Color{0.0f, 1.0f, 0.0f, 1.0f};
    }
    if (name == "red") {
        return Color{1.0f, 0.0f, 0.0f, 1.0f};
    }
    if (name == "yellow") {
        return Color{1.0f, 1.0f, 0.0f, 1.0f};
    }
    if (name == "purple") {
        return Color{kPurpleRed, 0.0f, 1.0f, 1.0f};
    }
    if (name == "null") {
        return Color{1.0f, 1.0f, 1.0f, 1.0f};
    }
    return Color{0.0f, 1.0f, 1.0f, 1.0f};
}

HxStr NextHudName() {
    return HxStr(Rnd::MakeString("<hud%04d>", ++g_nHudNameCounter));
}
