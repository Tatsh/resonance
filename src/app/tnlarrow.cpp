#include "app/tnlarrow.h"

#include "app/tnlplayer.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/view.h"

namespace {

// Expiry frame of a hidden slot, far beyond a song's length.
constexpr float kNoFrame = 1e9f;

} // namespace

// 0x0043ff10
TnlArrow::TnlArrow(int nIndex) {
    mMesh =
        dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(FormatString("arrow%d.mesh", nIndex))));
    for (int i = 0; i < kSlotCount; ++i) {
        mViews[i] = nullptr;
        mExpireFrames[i] = kNoFrame;
    }
}

// 0x00456e20
TnlArrow::~TnlArrow() {
    for (int i = 0; i < kSlotCount; ++i) {
        Hide(i);
    }
}

// 0x00456e90
void TnlArrow::Show(TnlPlayer *pPlayer, float flExpireFrame) {
    const int nSlot = pPlayer->mPlayerNum - 1;
    if (mViews[nSlot] != nullptr) {
        Hide(nSlot);
    }
    Rnd::View *pView = pPlayer->mLocalView;
    pView->AddDraw(mMesh, nullptr);
    mViews[nSlot] = pView;
    mExpireFrames[nSlot] = flExpireFrame;
}

// 0x00456f20
void TnlArrow::Hide(int nSlot) {
    if (mViews[nSlot] != nullptr) {
        mViews[nSlot]->RemoveDraw(mMesh);
    }
    mViews[nSlot] = nullptr;
}

// 0x00456f68
void TnlArrow::SetFrame(float flFrame) {
    for (int i = 0; i < kSlotCount; ++i) {
        if (mExpireFrames[i] < flFrame) {
            Hide(i);
            mExpireFrames[i] = kNoFrame;
        }
    }
}
