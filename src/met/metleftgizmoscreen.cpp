#include "met/metleftgizmoscreen.h"

namespace {

static const char *const kScreenName = "lll";
static const char *const kDirectory = "metagame/shared";
static const char *const kContainerName = "left_panel_large";

static const char *const kGizmoView = "lll_gizmo.view";
static const char *const kKaleidoscopeView = "lll_gizmo_kscope.view";

enum { kGizmoViewIndex, kKaleidoscopeViewIndex, kViewCount };

} // namespace

MetLeftGizmoScreen::MetLeftGizmoScreen(MetRenderer *pRenderer, int nPriority)
    : MetGizmoPanel(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mViewNames.resize(kViewCount);
    mViewNames[kGizmoViewIndex] = kGizmoView;
    mViewNames[kKaleidoscopeViewIndex] = kKaleidoscopeView;
}

MetLeftGizmoScreen::~MetLeftGizmoScreen() {
}

MetLeftGizmoScreen *MetLeftGizmoScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLeftGizmoScreen(pRenderer, nPriority);
}
