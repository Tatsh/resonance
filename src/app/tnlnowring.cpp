#include "app/tnlnowring.h"

#include <cmath>
#include <cstring>

#include "math/vector3.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/transformable.h"
#include "rnd/view.h"

namespace {

// Frame the pending reset waits for, one bar before the song starts.
constexpr float kResetFrame = -1920.0f;

constexpr float kDegreesPerStep = 45.0f;
constexpr float kDegreesPerHalfTurn = 180.0f;

// One unit in the last place below the float nearest pi, as the binary stores it.
constexpr float kPi = 3.1415925f;

} // namespace

TnlNowRing::TnlNowRing(int nMeshCount, int nPlayerCount) : mResetPending(1) {
    mView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("nowring.view")));
    mRotView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("nowring rot.view")));

    mMeshes.resize(nMeshCount);
    for (int i = 0; i < nMeshCount; ++i) {
        mMeshes[i] =
            dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(FormatString("nowmesh%d", i))));
        mMeshes[i]->SetShowing(1);
    }
    mPlayerMeshes.resize(nPlayerCount, 0);
}

void TnlNowRing::SetPlayerMesh(int nPlayer, int nMesh) {
    mPlayerMeshes[nPlayer] = nMesh;
    if (mResetPending == 0) {
        RefreshMeshes();
    }
}

void TnlNowRing::SetFrame(float flFrame) {
    if (mResetPending == 0 || !(kResetFrame < flFrame)) {
        return;
    }
    RefreshMeshes();
    SetRotation(0);
    mResetPending = 0;
}

void TnlNowRing::SetRotation(int nStep) {
    if (mResetPending == 0) {
        return;
    }
    const float flAngle = static_cast<float>(-nStep) * kDegreesPerStep * kPi / kDegreesPerHalfTurn;
    const float flCos = cosf(flAngle);
    const float flSin = sinf(flAngle);
    const Vector3 basis[] = {
        {flCos, 0.0f, -flSin, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {flSin, 0.0f, flCos, 1.0f},
    };
    std::memcpy(mRotView->mLocalXfm, basis, sizeof(basis));
    mRotView->mDirty = 1;
}

void TnlNowRing::RefreshMeshes() {
    for (std::vector<Rnd::Mesh *>::iterator it = mMeshes.begin(); it != mMeshes.end(); ++it) {
        (*it)->SetShowing(1);
    }
    for (std::vector<int>::iterator it = mPlayerMeshes.begin(); it != mPlayerMeshes.end(); ++it) {
        mMeshes[*it]->SetShowing(0);
    }
}
