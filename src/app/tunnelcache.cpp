#include "app/tunnelcache.h"

#include "os/hxstr.h"
#include "os/log.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/transformable.h"
#include "rnd/tunnel.h"
#include "rnd/view.h"

namespace {

constexpr char kTunnelObjectName[] = "tunnel";
constexpr char kScaleOffsetMissingFormat[] = "%s must have scale-offset filter in slot 1";
constexpr char kMinMaxLoopMissingFormat[] = "%s must have min-max-loop filter in last slot";

const char *NameText(const Rnd::Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : g_szEmptyString;
}

} // namespace

// 0x006de808
Rnd::Tunnel *g_pTunnel;

// 0x0040d1e8
void CacheTunnelObjectByName() {
    g_pTunnel = dynamic_cast<Rnd::Tunnel *>(Rnd::g_manager.Find(HxStr(kTunnelObjectName)));
}

// 0x0040f6c0
Rnd::Tunnel *GetCachedTunnelObject() {
    return g_pTunnel;
}

namespace Rnd {

// 0x0040d2b0
void Animatable::SetRate(float flRate) {
    ScaleOffset *pStage = nullptr;
    if (mFilters.size() != 0) {
        pStage = dynamic_cast<ScaleOffset *>(mFilters.front());
    }
    if (pStage == nullptr) {
        Fatal(kScaleOffsetMissingFormat, NameText(this));
    }

    const float flOutput = pStage->Apply(mFrame);
    pStage->mScale = flRate;
    pStage->mOffset = flOutput - mFrame * flRate;
}

// 0x0040d3d0
void Animatable::SetOffset(float flOffset) {
    ScaleOffset *pStage = nullptr;
    if (mFilters.size() != 0) {
        pStage = dynamic_cast<ScaleOffset *>(mFilters.front());
    }
    if (pStage == nullptr) {
        Fatal(kScaleOffsetMissingFormat, NameText(this));
    }

    pStage->mOffset = flOffset;
}

// 0x0040d4a8
void Animatable::SetLoopRange(float flMin, float flMax) {
    MinMaxLoop *pStage = nullptr;
    if (mFilters.size() != 0) {
        pStage = dynamic_cast<MinMaxLoop *>(mFilters.back());
    }
    if (pStage == nullptr) {
        Fatal(kMinMaxLoopMissingFormat, NameText(this));
    }

    pStage->mMax = flMax;
    pStage->mMin = flMin;
}

// 0x0040f660
void View::AddView(View *pChild) {
    AddDraw(pChild, nullptr);
    AddAnim(pChild);
    AddTrans(pChild);
}

// 0x0040f5f8
void View::RemoveView(View *pChild) {
    RemoveDraw(pChild);
    RemoveAnim(pChild);
    RemoveTrans(pChild);
    RemoveCollide(pChild);
}

} // namespace Rnd
