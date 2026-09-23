#include "met/metendgamegizmoscreen.h"

#include "rnd/manager.h"
#include "rnd/view.h"

namespace {

static const char *const kScreenName = "egg";
static const char *const kDirectory = "metagame/shared";
static const char *const kContainerName = "end_game_gizmo";

static const char *const kEqualizerView = "egg_eq.view";

enum { kEqualizerViewIndex, kViewCount };

} // namespace

// 0x002779f0
MetEndGameGizmoScreen::MetEndGameGizmoScreen(MetRenderer *pRenderer, int nPriority)
    : MetGizmoPanel(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mViewNames.resize(kViewCount);
    mViewNames[kEqualizerViewIndex] = kEqualizerView;
}

// 0x0027bd58
MetEndGameGizmoScreen::~MetEndGameGizmoScreen() {
}

// 0x0027bec8
MetEndGameGizmoScreen *MetEndGameGizmoScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetEndGameGizmoScreen(pRenderer, nPriority);
}

// 0x0027bfd0
void MetEndGameGizmoScreen::BeginExit() {
    dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(mViewNames[kEqualizerViewIndex]))->SetShowing(0);
    MetScreen::BeginExit();
}

// 0x0027bf50
void MetEndGameGizmoScreen::OnUnknownSlot33() {
    dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(mViewNames[kEqualizerViewIndex]))->SetShowing(1);
}
