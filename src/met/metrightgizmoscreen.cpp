#include "met/metrightgizmoscreen.h"

#include "rnd/manager.h"
#include "rnd/view.h"

namespace {

static const char *const kScreenName = "rpl";
static const char *const kDirectory = "metagame/shared";
static const char *const kContainerName = "right_panel_large";

static const char *const kGizmoView = "rpl_gizmo.view";
static const char *const kEqualizerView = "rpl_gizmo_eq.view";
static const char *const kKaleidoscopeView = "rpl_gizmo_kscope.view";

enum { kGizmoViewIndex, kEqualizerViewIndex, kKaleidoscopeViewIndex, kViewCount };

} // namespace

MetRightGizmoScreen::MetRightGizmoScreen(MetRenderer *pRenderer, int nPriority)
    : MetGizmoPanel(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mViewNames.resize(kViewCount);
    mViewNames[kGizmoViewIndex] = kGizmoView;
    mViewNames[kEqualizerViewIndex] = kEqualizerView;
    mViewNames[kKaleidoscopeViewIndex] = kKaleidoscopeView;
}

MetRightGizmoScreen::~MetRightGizmoScreen() {
}

MetRightGizmoScreen *MetRightGizmoScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetRightGizmoScreen(pRenderer, nPriority);
}

void MetRightGizmoScreen::BeginExit() {
    dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(mViewNames[kEqualizerViewIndex]))->SetShowing(0);
    MetScreen::BeginExit();
}

void MetRightGizmoScreen::OnEnterFinished() {
    dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(mViewNames[kEqualizerViewIndex]))->SetShowing(1);
}
