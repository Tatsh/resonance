#include "met/metleftgizmosmallscreen.h"

namespace {

static const char *const kScreenName = "lls";
static const char *const kDirectory = "metagame/shared";
static const char *const kContainerName = "left_panel_small";

static const char *const kGyroView = "lls_gyro.view";
static const char *const kKaleidoscopeView = "lls_gizmo_kscope.view";

enum { kGyroViewIndex, kKaleidoscopeViewIndex, kViewCount };

} // namespace

MetLeftGizmoSmallScreen::MetLeftGizmoSmallScreen(MetRenderer *pRenderer, int nPriority)
    : MetGizmoPanel(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mViewNames.resize(kViewCount);
    mViewNames[kGyroViewIndex] = kGyroView;
    mViewNames[kKaleidoscopeViewIndex] = kKaleidoscopeView;
}

MetLeftGizmoSmallScreen::~MetLeftGizmoSmallScreen() {
}

MetLeftGizmoSmallScreen *MetLeftGizmoSmallScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLeftGizmoSmallScreen(pRenderer, nPriority);
}
