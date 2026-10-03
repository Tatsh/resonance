#include "met/metleftgizmoscreen.h"

namespace {

static const char *const kScreenName = "lll";
static const char *const kDirectory = "metagame/shared";
static const char *const kContainerName = "left_panel_large";

static const char *const kGizmoView = "lll_gizmo.view";
static const char *const kKaleidoscopeView = "lll_gizmo_kscope.view";

enum { kGizmoViewIndex, kKaleidoscopeViewIndex, kViewCount };

} // namespace

// NTSC-U/C: 0x00276ed8, PAL: 0x0028f830
MetLeftGizmoScreen::MetLeftGizmoScreen(MetRenderer *pRenderer, int nPriority)
    : MetGizmoPanel(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mViewNames.resize(kViewCount);
    mViewNames[kGizmoViewIndex] = kGizmoView;
    mViewNames[kKaleidoscopeViewIndex] = kKaleidoscopeView;
}

// NTSC-U/C: 0x0027b4b8, PAL: 0x00294170
MetLeftGizmoScreen::~MetLeftGizmoScreen() {
}

// NTSC-U/C: 0x0027b628, PAL: 0x002942f8
MetLeftGizmoScreen *MetLeftGizmoScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLeftGizmoScreen(pRenderer, nPriority);
}
