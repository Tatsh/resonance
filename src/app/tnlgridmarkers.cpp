#include "app/tnlgridmarkers.h"

#include <cmath>
#include <cstring>

#include "app/apptunnel.h"
#include "app/tnlname.h"
#include "app/tnlutil.h"
#include "app/tunnelcache.h"
#include "math/color.h"
#include "math/transform.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/object.h"
#include "rnd/transformable.h"
#include "rnd/tunnel.h"
#include "rnd/view.h"

namespace {

constexpr int kFramesPerBar = 1920;
constexpr float kFramesPerBarFloat = 1920.0f;
constexpr float kFramesPerBeat = 480.0f;

constexpr int kMarkerCount = 16;
constexpr float kMarkerSpacing = 240.0f;

// Length of the whole row of markers, in frames.
constexpr float kRowLength = 3840.0f;

// Distance behind the song position at which a marker moves to the front of the row.
constexpr float kRecycleDistance = 480.0f;

constexpr float kMaxAlpha = 0.8f;

constexpr float kCentreLane = 0.5f;
constexpr float kTangentScale = 0.97f;

} // namespace

TnlGridMarkers::Marker::~Marker() {
    delete mMesh;
}

void TnlGridMarkers::Marker::Init(Rnd::Mesh *pSource, Rnd::Drawable *pParent) {
    if (!pSource) {
        mMesh = nullptr;
        return;
    }
    mMesh = Rnd::g_pfnNewMesh(NextAppTunnelName());
    // The binary does not set the flags argument. The 0 written here is not recovered.
    static_cast<Rnd::Object *>(mMesh)->Copy(pSource, 0);
    pParent->AddDraw(mMesh, nullptr);
}

void TnlGridMarkers::Marker::Place(int nTrack, int nFrame) {
    if (!mMesh) {
        return;
    }
    Rnd::Transformable *pTrans = mMesh;
    const float flFrame = static_cast<float>(nFrame);
    Transform xfm;
    PadTransformRows(xfm);
    GetCachedTunnelObject()->ProjectSectionToCameraSpace(
        nTrack, &xfm, flFrame, kCentreLane, kTangentScale);
    std::memcpy(pTrans->mLocalXfm, &xfm, sizeof(pTrans->mLocalXfm));
    pTrans->mDirty = 1;
    pTrans->UpdateWorldXfm(nullptr, 0);
    mFrame = flFrame;
}

TnlGridMarkers::TnlGridMarkers(AppTunnel *pTunnel, int nPlayerNum) : mTrack(0), mTunnel(pTunnel) {
    Rnd::View *pView = dynamic_cast<Rnd::View *>(
        Rnd::g_manager.Find(HxStr(FormatString("grid%d.view", nPlayerNum))));
    if (!pView) {
        return;
    }
    Rnd::Mesh *pBeatMesh = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr("grid4")));
    Rnd::Mesh *pOffBeatMesh = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr("grid8")));
    pView->ClearDraws();
    mMarkers.resize(kMarkerCount);
    float flFrame = 0.0f;
    for (auto it = mMarkers.begin(); it != mMarkers.end(); ++it) {
        // The bar test is redundant with the beat test, and the binary makes both.
        const bool bOnBeat = std::fmod(flFrame, kFramesPerBarFloat) == 0.0f ||
                             std::fmod(flFrame, kFramesPerBeat) == 0.0f;
        it->Init(bOnBeat ? pBeatMesh : pOffBeatMesh, pView);
        it->Place(mTrack, static_cast<int>(flFrame));
        flFrame += kMarkerSpacing;
    }
}

void TnlGridMarkers::SetTrack(int nTrack) {
    for (auto it = mMarkers.begin(); it != mMarkers.end(); ++it) {
        it->Place(nTrack, static_cast<int>(it->mFrame));
    }
    mTrack = nTrack;
}

void TnlGridMarkers::Update(float flFrame) {
    for (auto it = mMarkers.begin(); it != mMarkers.end(); ++it) {
        if (kRecycleDistance < flFrame - it->mFrame) {
            it->Place(mTrack, static_cast<int>(it->mFrame + kRowLength));
        }
        if (it->mMesh) {
            float flAlpha = (kRowLength - it->mFrame + flFrame) / kFramesPerBarFloat;
            if (kMaxAlpha < flAlpha) {
                flAlpha = kMaxAlpha;
            }
            it->mMesh->SetVertexColor(Color{1.0f, 1.0f, 1.0f, flAlpha});
            const int nBar = static_cast<int>(it->mFrame) / kFramesPerBar;
            it->mMesh->SetShowing(!mTunnel->IsTrackBarLocked(mTrack, nBar));
        }
    }
}
