#include "met/mettoplogoscreen.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/view.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "lp";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "freq_logo_panel";

// The two views ResolveContainerViews() resolves.
static const char *const kPanelView = "logo_panel.view";
static const char *const kWaveView = "wave.view";

// Resolves a registry key to a Rnd::View, or null.
inline Rnd::View *FindView(const HxStr &name) {
    Rnd::Object *pObject = Rnd::g_manager.Find(name);
    return pObject != nullptr ? dynamic_cast<Rnd::View *>(pObject) : nullptr;
}

} // namespace

// NTSC-U/C: 0x003c46f8, PAL: 0x003fb828
MetTopLogoScreen::MetTopLogoScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mShowsLoadedDrawables = 0;
}

// NTSC-U/C: 0x003c7710, PAL: 0x003fe980
MetTopLogoScreen *MetTopLogoScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetTopLogoScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x003c7798, PAL: 0x003fea08
MetTopLogoScreen::~MetTopLogoScreen() {
}

// NTSC-U/C: 0x003c77f0, PAL: 0x003fea60
void MetTopLogoScreen::UpdateIdle(float flTime) {
    mWaveView->SetFrame(flTime);
}

// NTSC-U/C: 0x003c4868, PAL: 0x003fba00
void MetTopLogoScreen::ResolveContainerViews() {
    ResolveAnimationViews();
    mView = FindView(HxStr(kPanelView));
    mView->ReleaseAnimsRefs(); // The binary does not test the view for null.
    mViewsUnresolved = 0;
    mWaveView = FindView(HxStr(kWaveView));
}
