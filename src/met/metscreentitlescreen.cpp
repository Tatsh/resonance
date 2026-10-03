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

// NTSC-U/C: 0x00390e10, PAL: 0x003c2908
MetScreenTitleScreen::MetScreenTitleScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
}

// NTSC-U/C: 0x00393fa0, PAL: 0x003c5a18
MetScreenTitleScreen::~MetScreenTitleScreen() {
}

// NTSC-U/C: 0x00393d78, PAL: 0x003c5990
MetScreenTitleScreen *MetScreenTitleScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetScreenTitleScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x00393e00, PAL: 0x003c2728
void MetScreenTitleScreen::SetTitle(const HxStr &title) {
    // Yes, the binary calls through the cast result without testing it for null.
    FindTitleScreen()->ApplyTitle(title);
}

// NTSC-U/C: 0x00393ed0, PAL: 0x003c2818
void MetScreenTitleScreen::ReplaceTitle(const HxStr &title) {
    // Yes, the binary calls through the cast result without testing it for null.
    FindTitleScreen()->ReplaceTitleText(title);
}

// NTSC-U/C: 0x00394010, PAL: 0x003c5a98
void MetScreenTitleScreen::ApplyTitle(const HxStr &title) {
    mTitle = title;
    PushNamedScreen(HxStr(kOwnScreenName));
}

// NTSC-U/C: 0x00394130, PAL: 0x003c5bd8
void MetScreenTitleScreen::ReplaceTitleText(const HxStr &title) {
    mTitle = title;
    mTitleText->SetText(mTitle);
}

// NTSC-U/C: 0x003940b8, PAL: 0x003c5b60
void MetScreenTitleScreen::EnterAndShow() {
    mTitleText->SetText(mTitle);
    MetScreen::EnterAndShow();
}

// NTSC-U/C: 0x00394108, PAL: 0x003c5bb0
void MetScreenTitleScreen::BeginExit() {
    MetScreen::BeginExit();
}

// NTSC-U/C: 0x00394100, PAL: 0x003c5ba8
void MetScreenTitleScreen::OnEnterFinished() {
}

// NTSC-U/C: 0x00394128, PAL: 0x003c5bd0
void MetScreenTitleScreen::OnExitFinished() {
}

// NTSC-U/C: 0x00390f88, PAL: 0x003c2ae8
void MetScreenTitleScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    Rnd::Object *pObject = Rnd::g_manager.Find(HxStr(kTitleText));
    mTitleText = pObject != nullptr ? dynamic_cast<Rnd::Text *>(pObject) : nullptr;
}
