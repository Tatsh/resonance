#include "met/metkeyboardscreen.h"

#include "app/playsound.h"
#include "os/hxstr.h"

namespace {

// Posted to the ticker when the keyboard departs.
static const char *const kClearTickerTemplate = "keyboard_clear_ticker";
// The two key clicks. The first accompanies sliding and the second every other navigation.
static const char *const kKeySound1 = "SND_MET_KEY1";
static const char *const kKeySound2 = "SND_MET_KEY2";

// mSelector when the screen accepts every selector.
constexpr int kSelectorAny = -1;

} // namespace

void MetKeyboardScreen::BeginExit() {
    SetTickerText(HxStr(kClearTickerTemplate));
    MetScreen::BeginExit();
}

void MetKeyboardScreen::PlaySlideSound(int nSelector) {
    if (mSelector == nSelector || mSelector == kSelectorAny) {
        PlaySoundByName(kKeySound1);
    }
}

void MetKeyboardScreen::PlayHighSound(int nSelector) {
    if (mSelector == nSelector || mSelector == kSelectorAny) {
        PlaySoundByName(kKeySound2);
    }
}

void MetKeyboardScreen::PlayCycleLeftSound(int nSelector) {
    if (mSelector == nSelector || mSelector == kSelectorAny) {
        PlaySoundByName(kKeySound2);
    }
}

void MetKeyboardScreen::PlayCycleRightSound(int nSelector) {
    if (mSelector == nSelector || mSelector == kSelectorAny) {
        PlaySoundByName(kKeySound2);
    }
}

void MetKeyboardScreen::OnUnknownSlot33() {
    SetTickerText(mUnknownc4);
}
