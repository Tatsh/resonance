#include "met/metgizmopanel.h"

#include "rnd/animatable.h"
#include "rnd/manager.h"
#include "rnd/view.h"

MetGizmoPanel::MetGizmoPanel(MetRenderer *pRenderer,
                             int nPriority,
                             const HxStr &name,
                             const HxStr &directory,
                             const HxStr &container)
    : MetScreen(pRenderer, nPriority, name, directory, container) {
}

MetGizmoPanel::~MetGizmoPanel() {
}

void MetGizmoPanel::UpdateIdle(float flTime) {
    int nCount = mViews.size();
    for (int i = 0; i < nCount; ++i) {
        mViews[i]->SetFrame(flTime);
    }
}

void MetGizmoPanel::UpdateIdleAnimation(float flTime) {
    UpdateIdle(flTime);
}

void MetGizmoPanel::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    int nCount = mViewNames.size();
    mViews.resize(nCount);
    for (int i = 0; i < nCount; ++i) {
        mViews[i] = dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(mViewNames[i]));
    }
}
