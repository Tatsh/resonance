#include "met/metleftgizmosmallscreen.h"

namespace {

static const char *const kScreenName = "lls";
static const char *const kDirectory = "metagame/shared";
static const char *const kContainerName = "left_panel_small";

static const char *const kGyroView = "lls_gyro.view";
static const char *const kKaleidoscopeView = "lls_gizmo_kscope.view";

enum { kGyroViewIndex, kKaleidoscopeViewIndex, kViewCount };

} // namespace

// 0x00277280
MetLeftGizmoSmallScreen::MetLeftGizmoSmallScreen(MetRenderer *pRenderer, int nPriority)
    : MetGizmoPanel(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mViewNames.resize(kViewCount);
    mViewNames[kGyroViewIndex] = kGyroView;
    mViewNames[kKaleidoscopeViewIndex] = kKaleidoscopeView;
}

// 0x0027b738
MetLeftGizmoSmallScreen::~MetLeftGizmoSmallScreen() {
}

// 0x0027b8a8
MetLeftGizmoSmallScreen *MetLeftGizmoSmallScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLeftGizmoSmallScreen(pRenderer, nPriority);
}
