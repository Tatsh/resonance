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

MetEndGameGizmoScreen::MetEndGameGizmoScreen(MetRenderer *pRenderer, int nPriority)
    : MetGizmoPanel(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mViewNames.resize(kViewCount);
    mViewNames[kEqualizerViewIndex] = kEqualizerView;
}

MetEndGameGizmoScreen::~MetEndGameGizmoScreen() {
}

MetEndGameGizmoScreen *MetEndGameGizmoScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetEndGameGizmoScreen(pRenderer, nPriority);
}

void MetEndGameGizmoScreen::BeginExit() {
    dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(mViewNames[kEqualizerViewIndex]))->SetShowing(0);
    MetScreen::BeginExit();
}

void MetEndGameGizmoScreen::OnEnterFinished() {
    dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(mViewNames[kEqualizerViewIndex]))->SetShowing(1);
}
