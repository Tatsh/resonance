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

// 0x003c46f8
MetTopLogoScreen::MetTopLogoScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mUnknown60 = 0;
}

// 0x003c7710
MetTopLogoScreen *MetTopLogoScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetTopLogoScreen(pRenderer, nPriority);
}

// 0x003c7798
MetTopLogoScreen::~MetTopLogoScreen() {
}

// 0x003c77f0
void MetTopLogoScreen::OnUnknownSlot26(float flTime) {
    mUnknown8c->SetFrame(flTime);
}

// 0x003c4868
void MetTopLogoScreen::ResolveContainerViews() {
    ResolveAnimationViews();
    mUnknown14 = FindView(HxStr(kPanelView));
    mUnknown14->ReleaseAnimsRefs(); // The binary does not test the view for null.
    mUnknown48 = 0;
    mUnknown8c = FindView(HxStr(kWaveView));
}
