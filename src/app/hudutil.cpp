#include "app/hudutil.h"

#include "os/formatstring.h"

namespace {

// Share of full red in the colour `purple` selects.
constexpr float kPurpleRed = 0.65f;

} // namespace

// 0x006dfde8
int g_nHudNameCounter;

// 0x00415ed8
HxStr HudPowerupName(int nKind) {
    switch (nKind) {
    case kHudItemNeutralizer:
        return HxStr("NEUTRALIZER");
    case kHudItemCrippler:
        return HxStr("CRIPPLER");
    case kHudItemFreestyler:
        return HxStr("FREESTYLER");
    case kHudItemAutocatcher:
        return HxStr("AUTOCATCHER");
    case kHudItemBumper:
        return HxStr("BUMPER");
    case kHudItemVolume:
        return HxStr("Volume");
    case kHudItemWah:
        return HxStr("Wah");
    case kHudItemStutter:
        return HxStr("Stutter");
    case kHudItemEcho:
        return HxStr("Echo");
    case kHudItemFlange:
        return HxStr("Flange");
    case kHudItemChorus:
        return HxStr("Chorus");
    case kHudItemGuides:
        return HxStr("Guides");
    case kHudItemMultiplier:
        return HxStr("MULTIPLIER");
    default:
        return HxStr("");
    }
}

// 0x00416068
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

// 0x004298e8
HxStr NextHudName() {
    return HxStr(FormatString("<hud%04d>", ++g_nHudNameCounter));
}
