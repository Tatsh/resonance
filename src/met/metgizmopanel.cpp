#include "met/metgizmopanel.h"

#include "rnd/animatable.h"
#include "rnd/manager.h"
#include "rnd/view.h"

// 0x00276d38
MetGizmoPanel::MetGizmoPanel(MetRenderer *pRenderer,
                             int nPriority,
                             const HxStr &name,
                             const HxStr &directory,
                             const HxStr &container)
    : MetScreen(pRenderer, nPriority, name, directory, container) {
}

// 0x0027b218
MetGizmoPanel::~MetGizmoPanel() {
}

// 0x0027b3b0
void MetGizmoPanel::OnUnknownSlot26(float flTime) {
    int nCount = mViews.size();
    for (int i = 0; i < nCount; ++i) {
        mViews[i]->SetFrame(flTime);
    }
}

// 0x0027b388
void MetGizmoPanel::UpdateIdleAnimation(float flTime) {
    OnUnknownSlot26(flTime);
}

// 0x00276d90
void MetGizmoPanel::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    int nCount = mViewNames.size();
    mViews.resize(nCount);
    for (int i = 0; i < nCount; ++i) {
        mViews[i] = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(mViewNames[i]));
    }
}
