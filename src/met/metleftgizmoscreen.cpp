#include "met/metleftgizmoscreen.h"

namespace {

static const char *const kScreenName = "lll";
static const char *const kDirectory = "metagame/shared";
static const char *const kContainerName = "left_panel_large";

static const char *const kGizmoView = "lll_gizmo.view";
static const char *const kKaleidoscopeView = "lll_gizmo_kscope.view";

enum { kGizmoViewIndex, kKaleidoscopeViewIndex, kViewCount };

} // namespace

// 0x00276ed8
MetLeftGizmoScreen::MetLeftGizmoScreen(MetRenderer *pRenderer, int nPriority)
    : MetGizmoPanel(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mViewNames.resize(kViewCount);
    mViewNames[kGizmoViewIndex] = kGizmoView;
    mViewNames[kKaleidoscopeViewIndex] = kKaleidoscopeView;
}

// 0x0027b4b8
MetLeftGizmoScreen::~MetLeftGizmoScreen() {
}

// 0x0027b628
MetLeftGizmoScreen *MetLeftGizmoScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLeftGizmoScreen(pRenderer, nPriority);
}
