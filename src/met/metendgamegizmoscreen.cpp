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

// NTSC-U/C: 0x002779f0, PAL: 0x00290520
MetEndGameGizmoScreen::MetEndGameGizmoScreen(MetRenderer *pRenderer, int nPriority)
    : MetGizmoPanel(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mViewNames.resize(kViewCount);
    mViewNames[kEqualizerViewIndex] = kEqualizerView;
}

// NTSC-U/C: 0x0027bd58, PAL: 0x00294a58
MetEndGameGizmoScreen::~MetEndGameGizmoScreen() {
}

// NTSC-U/C: 0x0027bec8, PAL: 0x00294be0
MetEndGameGizmoScreen *MetEndGameGizmoScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetEndGameGizmoScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x0027bfd0, PAL: 0x00294ce8
void MetEndGameGizmoScreen::BeginExit() {
    dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(mViewNames[kEqualizerViewIndex]))->SetShowing(0);
    MetScreen::BeginExit();
}

// NTSC-U/C: 0x0027bf50, PAL: 0x00294c68
void MetEndGameGizmoScreen::OnEnterFinished() {
    dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(mViewNames[kEqualizerViewIndex]))->SetShowing(1);
}
