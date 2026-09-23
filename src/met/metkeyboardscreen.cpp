#include "met/metkeyboardscreen.h"

#include "app/playsound.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

// 0x006a7c80
std::vector<HxStr> g_defaultMacros;

// The number of default macros, one for each function key.
constexpr int kDefaultMacroCount = 12;
static const char *const kMacroKeyFormat = "kb_macro_f%i";
constexpr int kMacroConfigCode = 0x258;

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

std::vector<HxStr> *MetKeyboardScreen::GetDefaultMacros() {
    if (g_defaultMacros.size() != kDefaultMacroCount) {
        g_defaultMacros.resize(kDefaultMacroCount);
        for (int i = 0; i < kDefaultMacroCount; ++i) {
            g_defaultMacros[i] = DefaultMacro(i);
        }
    }
    return &g_defaultMacros;
}

HxStr MetKeyboardScreen::DefaultMacro(int nIndex) {
    HxStr text;
    QueryConfigString(&text, kMacroConfigCode, FormatString(kMacroKeyFormat, nIndex + 1));
    return text;
}
