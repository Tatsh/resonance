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

MetScreenTitleScreen::MetScreenTitleScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
}

MetScreenTitleScreen::~MetScreenTitleScreen() {
}

MetScreenTitleScreen *MetScreenTitleScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetScreenTitleScreen(pRenderer, nPriority);
}

void MetScreenTitleScreen::SetTitle(const HxStr &title) {
    // Yes, the binary calls through the cast result without testing it for null.
    FindTitleScreen()->ApplyTitle(title);
}

void MetScreenTitleScreen::ReplaceTitle(const HxStr &title) {
    // Yes, the binary calls through the cast result without testing it for null.
    FindTitleScreen()->ReplaceTitleText(title);
}

void MetScreenTitleScreen::ApplyTitle(const HxStr &title) {
    mTitle = title;
    PushNamedScreen(HxStr(kOwnScreenName));
}

void MetScreenTitleScreen::ReplaceTitleText(const HxStr &title) {
    mTitle = title;
    mTitleText->SetText(mTitle);
}

void MetScreenTitleScreen::EnterAndShow() {
    mTitleText->SetText(mTitle);
    MetScreen::EnterAndShow();
}

void MetScreenTitleScreen::BeginExit() {
    MetScreen::BeginExit();
}

void MetScreenTitleScreen::OnEnterFinished() {
}

void MetScreenTitleScreen::OnExitFinished() {
}

void MetScreenTitleScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    Rnd::Object *pObject = Rnd::TheManager.Find(HxStr(kTitleText));
    mTitleText = pObject != nullptr ? dynamic_cast<Rnd::Text *>(pObject) : nullptr;
}
