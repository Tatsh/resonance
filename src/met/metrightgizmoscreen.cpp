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

// 0x00277628
MetRightGizmoScreen::MetRightGizmoScreen(MetRenderer *pRenderer, int nPriority)
    : MetGizmoPanel(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mViewNames.resize(kViewCount);
    mViewNames[kGizmoViewIndex] = kGizmoView;
    mViewNames[kEqualizerViewIndex] = kEqualizerView;
    mViewNames[kKaleidoscopeViewIndex] = kKaleidoscopeView;
}

// 0x0027b9b8
MetRightGizmoScreen::~MetRightGizmoScreen() {
}

// 0x0027bb28
MetRightGizmoScreen *MetRightGizmoScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetRightGizmoScreen(pRenderer, nPriority);
}

// 0x0027bc38
void MetRightGizmoScreen::BeginExit() {
    dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(mViewNames[kEqualizerViewIndex]))->SetShowing(0);
    MetScreen::BeginExit();
}

// 0x0027bbb0
void MetRightGizmoScreen::OnUnknownSlot33() {
    dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(mViewNames[kEqualizerViewIndex]))->SetShowing(1);
}
