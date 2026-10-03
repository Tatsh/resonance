#include "met/metgizmopanel.h"

#include "rnd/animatable.h"
#include "rnd/manager.h"
#include "rnd/view.h"

// NTSC-U/C: 0x00276d38, PAL: 0x0028f690
MetGizmoPanel::MetGizmoPanel(MetRenderer *pRenderer,
                             int nPriority,
                             const HxStr &name,
                             const HxStr &directory,
                             const HxStr &container)
    : MetScreen(pRenderer, nPriority, name, directory, container) {
}

// NTSC-U/C: 0x0027b218, PAL: 0x00293eb8
MetGizmoPanel::~MetGizmoPanel() {
}

// NTSC-U/C: 0x0027b3b0, PAL: 0x00294068
void MetGizmoPanel::UpdateIdle(float flTime) {
    int nCount = mViews.size();
    for (int i = 0; i < nCount; ++i) {
        mViews[i]->SetFrame(flTime);
    }
}

// NTSC-U/C: 0x0027b388, PAL: 0x00294040
void MetGizmoPanel::UpdateIdleAnimation(float flTime) {
    UpdateIdle(flTime);
}

// NTSC-U/C: 0x00276d90, PAL: 0x0028f6e8
void MetGizmoPanel::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    int nCount = mViewNames.size();
    mViews.resize(nCount);
    for (int i = 0; i < nCount; ++i) {
        mViews[i] = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(mViewNames[i]));
    }
}
