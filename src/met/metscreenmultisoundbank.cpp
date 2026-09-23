#include "met/metscreenmultisoundbank.h"

#include "app/application.h"
#include "app/playsound.h"
#include "met/metscreen.h"
#include "os/hxstr.h"

namespace {

static const char *const kSlideSound = "SND_MET_SLIDE";
static const char *const kLeaveSound = "SND_MET_LEAVE";
static const char *const kHighSound = "SND_MET_HIGH";
static const char *const kCycleLeftSound = "SND_MET_CYCLE_L";
static const char *const kCycleRightSound = "SND_MET_CYCLE_R";
static const char *const kInGameActionSound = "SND_MET_MULTI_INGAME_ACTION";
static const char *const kInGameNavigationSound = "SND_MET_MULTI_INGAME_NAVIGATION";

// 0x003908f0
// Whether a game world exists, which is when the screen is shown over a running game and the
// in-game bank applies.
bool HasGameWorld() {
    return Application::shared()->GetWorld() != nullptr;
}

} // namespace

// 0x0038ff58
MetScreenMultiSoundBank::MetScreenMultiSoundBank(MetRenderer *pRenderer,
                                                 int nPriority,
                                                 const HxStr &name,
                                                 const HxStr &directory,
                                                 const HxStr &file)
    : MetScreen(pRenderer, nPriority, name, directory, file) {
}

// 0x0038fe60
MetScreenMultiSoundBank::~MetScreenMultiSoundBank() {
}

// 0x003907b0
void MetScreenMultiSoundBank::PlaySlideSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(HasGameWorld() ? kInGameActionSound : kSlideSound);
}

// 0x003907f0
void MetScreenMultiSoundBank::PlayLeaveSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(HasGameWorld() ? kInGameActionSound : kLeaveSound);
}

// 0x00390830
void MetScreenMultiSoundBank::PlayHighSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(HasGameWorld() ? kInGameNavigationSound : kHighSound);
}

// 0x00390870
void MetScreenMultiSoundBank::PlayCycleLeftSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(HasGameWorld() ? kInGameNavigationSound : kCycleLeftSound);
}

// 0x003908b0
void MetScreenMultiSoundBank::PlayCycleRightSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(HasGameWorld() ? kInGameNavigationSound : kCycleRightSound);
}
