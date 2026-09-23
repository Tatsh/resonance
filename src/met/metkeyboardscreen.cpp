#include "met/metkeyboardscreen.h"

#include <climits>

#include "app/playsound.h"
#include "met/metkeyboardrequest.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

// 0x006a7c80
std::vector<HxStr> g_defaultMacros;

// 0x006a7c8c
// The widest the entered text may measure, which Open() copies from the request.
int g_nKeyboardMaxWidth = 500;

// 0x006a7c90
// The most characters the entry accepts, which Open() copies from the request.
int g_nKeyboardMaxLength = INT_MAX;

// The screen the keyboard registers under.
static const char *const kKeyboardScreen = "MetKeyboardScreen";

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

// 0x0028c550
void MetKeyboardScreen::BeginExit() {
    SetTickerText(HxStr(kClearTickerTemplate));
    MetScreen::BeginExit();
}

// 0x0028c518
void MetKeyboardScreen::PlaySlideSound(int nSelector) {
    if (mSelector == nSelector || mSelector == kSelectorAny) {
        PlaySoundByName(kKeySound1);
    }
}

// 0x0028c4e0
void MetKeyboardScreen::PlayHighSound(int nSelector) {
    if (mSelector == nSelector || mSelector == kSelectorAny) {
        PlaySoundByName(kKeySound2);
    }
}

// 0x0028c470
void MetKeyboardScreen::PlayCycleLeftSound(int nSelector) {
    if (mSelector == nSelector || mSelector == kSelectorAny) {
        PlaySoundByName(kKeySound2);
    }
}

// 0x0028c4a8
void MetKeyboardScreen::PlayCycleRightSound(int nSelector) {
    if (mSelector == nSelector || mSelector == kSelectorAny) {
        PlaySoundByName(kKeySound2);
    }
}

// 0x0028c808
void MetKeyboardScreen::OnUnknownSlot33() {
    SetTickerText(mUnknownc4);
}

// 0x00284d30
std::vector<HxStr> *MetKeyboardScreen::GetDefaultMacros() {
    if (g_defaultMacros.size() != kDefaultMacroCount) {
        g_defaultMacros.resize(kDefaultMacroCount);
        for (int i = 0; i < kDefaultMacroCount; ++i) {
            g_defaultMacros[i] = DefaultMacro(i);
        }
    }
    return &g_defaultMacros;
}

// 0x0028cbb8
HxStr MetKeyboardScreen::DefaultMacro(int nIndex) {
    HxStr text;
    QueryConfigString(&text, kMacroConfigCode, FormatString(kMacroKeyFormat, nIndex + 1));
    return text;
}

// 0x00282468
void MetKeyboardScreen::Open(const MetKeyboardRequest &request) {
    // The binary narrows the lookup without a runtime check.
    MetKeyboardScreen *pKeyboard =
        static_cast<MetKeyboardScreen *>(MetScreen::FindScreenByName(HxStr(kKeyboardScreen)));
    pKeyboard->SetText(request.mText);
    pKeyboard->SetPrompt(request.mPrompt);
    pKeyboard->SetUser(request.mUser);
    pKeyboard->SetSelector(request.mPad);
    g_nKeyboardMaxWidth = request.mMaxWidth;
    g_nKeyboardMaxLength = request.mMaxLength;
    pKeyboard->mMacros = request.mMacros != nullptr ? request.mMacros : &g_defaultMacros;
    pKeyboard->SetTicker(request.mTicker);
    pKeyboard->SetReturnScreen(request.mReturnScreen);
    MetScreen *pReturn = MetScreen::FindScreenByName(request.mReturnScreen);
    pReturn->PushNamedScreen(HxStr(kKeyboardScreen));
    pReturn->ActivateNamedPanel(HxStr(kKeyboardScreen));
}

// 0x0028c830
void MetKeyboardScreen::SetText(const HxStr &text) {
    mUnknownb4 = text;
}

// 0x0028c850
void MetKeyboardScreen::SetPrompt(const HxStr &prompt) {
    mPrompt = prompt;
}

// 0x0028cad0
void MetKeyboardScreen::SetUser(MetKBUser *pUser) {
    mUser = pUser;
}

// 0x0028c3e0
void MetKeyboardScreen::SetSelector(int nSelector) {
    mSelector = nSelector;
}

// 0x0028cad8
void MetKeyboardScreen::SetTicker(const HxStr &ticker) {
    mUnknownc4 = ticker;
}

// 0x0028cab0
void MetKeyboardScreen::SetReturnScreen(const HxStr &returnScreen) {
    mUnknownac = returnScreen;
}
