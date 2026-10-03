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

// NTSC-U/C: 0x00277628, PAL: 0x002900b0
MetRightGizmoScreen::MetRightGizmoScreen(MetRenderer *pRenderer, int nPriority)
    : MetGizmoPanel(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mViewNames.resize(kViewCount);
    mViewNames[kGizmoViewIndex] = kGizmoView;
    mViewNames[kEqualizerViewIndex] = kEqualizerView;
    mViewNames[kKaleidoscopeViewIndex] = kKaleidoscopeView;
}

// NTSC-U/C: 0x0027b9b8, PAL: 0x002946a0
MetRightGizmoScreen::~MetRightGizmoScreen() {
}

// NTSC-U/C: 0x0027bb28, PAL: 0x00294828
MetRightGizmoScreen *MetRightGizmoScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetRightGizmoScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x0027bc38, PAL: 0x00294938
void MetRightGizmoScreen::BeginExit() {
    dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(mViewNames[kEqualizerViewIndex]))->SetShowing(0);
    MetScreen::BeginExit();
}

// NTSC-U/C: 0x0027bbb0, PAL: 0x002948b0
void MetRightGizmoScreen::OnEnterFinished() {
    dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(mViewNames[kEqualizerViewIndex]))->SetShowing(1);
}
