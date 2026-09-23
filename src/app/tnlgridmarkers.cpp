#include "app/tnlgridmarkers.h"

#include <cmath>

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

// 0x004546a0
// Deletes a marker's mesh. ~Marker() expands it inline, and this out-of-line copy has no caller.
inline void DeleteMesh(Rnd::Mesh *pMesh) {
    delete pMesh;
}

} // namespace

TnlGridMarkers::Marker::~Marker() {
    DeleteMesh(mMesh);
}

// 0x00438fe0
void TnlGridMarkers::Marker::Init(Rnd::Mesh *pSource, Rnd::Drawable *pParent) {
    if (!pSource) {
        mMesh = nullptr;
        return;
    }
    mMesh = Rnd::NewMeshThroughHook(NextAppTunnelName());
    static_cast<Rnd::Object *>(mMesh)->Copy(pSource, Rnd::Mesh::kCopyShareFaces);
    pParent->AddDraw(mMesh, nullptr);
}

// 0x00454fb0
void TnlGridMarkers::Marker::Place(int nTrack, int nFrame) {
    if (!mMesh) {
        return;
    }
    const float flFrame = static_cast<float>(nFrame);
    PlaceOnRing(*mMesh, nTrack, flFrame, kCentreLane);
    mFrame = flFrame;
}

// 0x00439150
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

// 0x004550c8
void TnlGridMarkers::SetTrack(int nTrack) {
    for (auto it = mMarkers.begin(); it != mMarkers.end(); ++it) {
        it->Place(nTrack, static_cast<int>(it->mFrame));
    }
    mTrack = nTrack;
}

// 0x00439718
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
