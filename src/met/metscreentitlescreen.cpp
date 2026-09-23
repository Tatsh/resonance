#include "met/metscreentitlescreen.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/text.h"

namespace {

// The screen name, the directory the container loads from, and the container name.
static const char *const kScreenName = "fst";
static const char *const kDirectory = "metagame/shared";
static const char *const kContainerName = "screen_title";

// This screen's registry key, and the title text the container holds.
static const char *const kOwnScreenName = "MetScreenTitleScreen";
static const char *const kTitleText = "fst_title.txt";

// Resolves this screen through the registry, or null when it is not registered.
inline MetScreenTitleScreen *FindTitleScreen() {
    MetScreen *pScreen = MetScreen::FindScreenByName(HxStr(kOwnScreenName));
    return pScreen != nullptr ? dynamic_cast<MetScreenTitleScreen *>(pScreen) : nullptr;
}

} // namespace

// 0x00390e10
MetScreenTitleScreen::MetScreenTitleScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
}

// 0x00393fa0
MetScreenTitleScreen::~MetScreenTitleScreen() {
}

// 0x00393d78
MetScreenTitleScreen *MetScreenTitleScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetScreenTitleScreen(pRenderer, nPriority);
}

// 0x00393e00
void MetScreenTitleScreen::SetTitle(const HxStr &title) {
    // Yes, the binary calls through the cast result without testing it for null.
    FindTitleScreen()->ApplyTitle(title);
}

// 0x00393ed0
void MetScreenTitleScreen::ReplaceTitle(const HxStr &title) {
    // Yes, the binary calls through the cast result without testing it for null.
    FindTitleScreen()->ReplaceTitleText(title);
}

// 0x00394010
void MetScreenTitleScreen::ApplyTitle(const HxStr &title) {
    mTitle = title;
    PushNamedScreen(HxStr(kOwnScreenName));
}

// 0x00394130
void MetScreenTitleScreen::ReplaceTitleText(const HxStr &title) {
    mTitle = title;
    mTitleText->SetText(mTitle);
}

// 0x003940b8
void MetScreenTitleScreen::EnterAndShow() {
    mTitleText->SetText(mTitle);
    MetScreen::EnterAndShow();
}

// 0x00394108
void MetScreenTitleScreen::BeginExit() {
    MetScreen::BeginExit();
}

// 0x00394100
void MetScreenTitleScreen::OnUnknownSlot33() {
}

// 0x00394128
void MetScreenTitleScreen::OnUnknownSlot36() {
}

// 0x00390f88
void MetScreenTitleScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    Rnd::Object *pObject = Rnd::g_manager.Find(HxStr(kTitleText));
    mTitleText = pObject != nullptr ? dynamic_cast<Rnd::Text *>(pObject) : nullptr;
}
