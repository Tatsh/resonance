#include "met/metleftgizmosmallscreen.h"

namespace {

static const char *const kScreenName = "lls";
static const char *const kDirectory = "metagame/shared";
static const char *const kContainerName = "left_panel_small";

static const char *const kGyroView = "lls_gyro.view";
static const char *const kKaleidoscopeView = "lls_gizmo_kscope.view";

enum { kGyroViewIndex, kKaleidoscopeViewIndex, kViewCount };

} // namespace

// NTSC-U/C: 0x00277280, PAL: 0x0028fc70
MetLeftGizmoSmallScreen::MetLeftGizmoSmallScreen(MetRenderer *pRenderer, int nPriority)
    : MetGizmoPanel(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mViewNames.resize(kViewCount);
    mViewNames[kGyroViewIndex] = kGyroView;
    mViewNames[kKaleidoscopeViewIndex] = kKaleidoscopeView;
}

// NTSC-U/C: 0x0027b738, PAL: 0x00294408
MetLeftGizmoSmallScreen::~MetLeftGizmoSmallScreen() {
}

// NTSC-U/C: 0x0027b8a8, PAL: 0x00294590
MetLeftGizmoSmallScreen *MetLeftGizmoSmallScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLeftGizmoSmallScreen(pRenderer, nPriority);
}
