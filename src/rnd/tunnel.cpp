#include "rnd/tunnel.h"

#include <vector>

#include "math/transform.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
#include "rnd/mesh.h"
#include "rnd/object.h"
#include "rnd/transanim.h"

namespace {

// The unset slice value, a hand-written sentinel.
constexpr int kNoSlice = 99999999;

// A signed remainder moved into [0, nCount), the form every ring and slice lookup uses.
inline int WrapIndex(int nIndex, int nCount) {
    const int nRemainder = nIndex % nCount;
    return nRemainder > -1 ? nRemainder : nRemainder + nCount;
}

} // namespace

namespace Rnd {

namespace {

// 0x00476ec0
void CollideMeshes(const std::vector<Mesh *> &meshes, const Ray &ray, Collideable::HitSink &sink) {
    for (Mesh *pMesh : meshes) {
        pMesh->Collide(ray, sink);
    }
}

} // namespace

// 0x00476a10
void Tunnel::Collide(const Ray &ray, HitSink &sink) {
    for (const std::vector<Mesh *> &meshes : mUnknowna4) {
        CollideMeshes(meshes, ray, sink);
    }
}

// 0x006eab10
HxStr g_tunnelClassName("Tunnel");

// 0x004763b8
const HxStr &Tunnel::ClassName() const {
    return g_tunnelClassName;
}

// 0x004768b8
void Tunnel::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Drawable::DumpText(sink);
    Animatable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    // The author never finished this block. It writes no member of the class, and the Collideable
    // base is not dumped either.
    sink.Print("[Tunnel]\n");
    sink.Print("TODO\n");
}

// 0x0046d180
void Tunnel::ApplyMeshLodScreenSizes(const std::vector<float> &screenSizes) {
    mUnknown68 = screenSizes;
    for (unsigned nSlice = 0; nSlice < mUnknowna4.size(); ++nSlice) {
        for (unsigned nRing = 0; nRing < mUnknowna4[nSlice].size(); ++nRing) {
            if (nRing < mUnknown68.size()) {
                Mesh *pMesh = mUnknowna4[nSlice][nRing];
                pMesh->mMinScreen = mUnknown68[nRing];
                // Yes, the binary releases and immediately re-takes the reference on the same
                // link, because the argument is the link the mesh already stores.
                pMesh->SetNext(pMesh->mNext);
            }
        }
    }
    for (unsigned nSlice = 0; nSlice < mUnknownb0.size(); ++nSlice) {
        for (unsigned nRing = 0; nRing < mUnknownb0[nSlice].size(); ++nRing) {
            if (nRing < mUnknown68.size()) {
                Mesh *pMesh = mUnknownb0[nSlice][nRing];
                pMesh->mMinScreen = mUnknown68[nRing];
                pMesh->SetNext(pMesh->mNext);
            }
        }
    }
}

// 0x0046acf0
void Tunnel::ClearMaterialSectionLists() {
    mUnknownb0.clear();
    mUnknowna4.clear();
}

// 0x0046db80
void Tunnel::ProjectSectionToCameraSpace(
    int nRing, Transform *pOut, float flAnimFrame, float flRingBlend, float flTangentScale) {
    if (mUnknown58 == nullptr) {
        // Yes, the padding words of the three basis rows are not written, while the translation
        // row is stored whole as (0, 0, 0, 1).
        pOut->mBasisX.x = 1.0f;
        pOut->mBasisX.y = 0.0f;
        pOut->mBasisX.z = 0.0f;
        pOut->mBasisY.x = 0.0f;
        pOut->mBasisY.y = 1.0f;
        pOut->mBasisY.z = 0.0f;
        pOut->mBasisZ.x = 0.0f;
        pOut->mBasisZ.y = 0.0f;
        pOut->mBasisZ.z = 1.0f;
        pOut->mTranslation.x = 0.0f;
        pOut->mTranslation.y = 0.0f;
        pOut->mTranslation.z = 0.0f;
        pOut->mTranslation.w = 1.0f;
        return;
    }
    const Transform &ring = mUnknownc0[nRing];
    pOut->mBasisX = ring.mBasisX;
    pOut->mBasisY = ring.mBasisY;
    pOut->mBasisZ = ring.mBasisZ;

    Vector3 current;
    current.w = 1.0f;
    Vec3Scale(&ring.mTranslation.x, flTangentScale, &current.x);
    Vector3 next;
    next.w = 1.0f;
    Vec3Scale(
        &mUnknownc0[WrapIndex(nRing + 1, mUnknown3c)].mTranslation.x, flTangentScale, &next.x);
    // A VU0 multiply and accumulate in the image, as in LerpRingSectionTangent().
    const float flComplement = 1.0f - flRingBlend;
    pOut->mTranslation.x = next.x * flRingBlend + current.x * flComplement;
    pOut->mTranslation.y = next.y * flRingBlend + current.y * flComplement;
    pOut->mTranslation.z = next.z * flRingBlend + current.z * flComplement;
    pOut->mTranslation.w = next.w;

    Transform anim;
    anim.mBasisX.w = 1.0f;
    anim.mBasisY.w = 1.0f;
    anim.mBasisZ.w = 1.0f;
    anim.mTranslation.w = 1.0f;
    mUnknown58->EvalFrame(flAnimFrame, &anim.mBasisX.x, 1);
    // Yes, the output is also the first input.
    XfmConcat(&pOut->mBasisX.x, &anim.mBasisX.x, &pOut->mBasisX.x);
}

// 0x004773e8
Mesh *Tunnel::GetRingSection(int nSlice) {
    return mUnknownb0[WrapIndex(nSlice, mUnknown40)].front();
}

// 0x00477388
Mesh *Tunnel::GetRingSection(int nRing, int nSlice) {
    return mUnknowna4[WrapIndex(nSlice, mUnknown40) * mUnknown3c + WrapIndex(nRing, mUnknown3c)]
        .front();
}

// 0x00477538. A VU0 multiply and accumulate in the image, vmulax then vmaddx over xyz.
void Tunnel::LerpRingSectionTangent(int nRing, Vector3 *pOut, float flWeight) {
    const Vector3 &next = mUnknownc0[WrapIndex(nRing + 1, mUnknown3c)].mTranslation;
    const Vector3 &current = mUnknownc0[nRing].mTranslation;
    const float flComplement = 1.0f - flWeight;
    pOut->x = next.x * flWeight + current.x * flComplement;
    pOut->y = next.y * flWeight + current.y * flComplement;
    pOut->z = next.z * flWeight + current.z * flComplement;
    pOut->w = next.w;
}

// 0x00476f48
void Tunnel::ScrollRings() {
    for (int nSlice = mUnknownbc; nSlice < mUnknownbc + mUnknown40; ++nSlice) {
        if (mUnknown88[WrapIndex(nSlice, mUnknown40)] != nSlice) {
            AdvanceRing(nSlice);
            return;
        }
    }
}

// 0x00476fe0
void Tunnel::AdvanceRing(int nSlice) {
    const int nIndex = WrapIndex(nSlice, mUnknown40);
    if (nSlice != mUnknown7c) {
        mUnknown88[nIndex] = kNoSlice;
        mUnknown7c = nSlice;
        mUnknown84 = mUnknowna0;
        mUnknown80 = nSlice * mUnknown9c;
    }
    SetRingSectionFrames();
    if (mUnknown84 == 0) {
        mUnknown88[nIndex] = nSlice;
        mUnknown7c = kNoSlice;
    } else {
        --mUnknown84;
        mUnknown80 += mUnknown9c / mUnknowna0;
    }
}

// 0x0046d400
void Tunnel::AddEvent(Drawable *pObject, float flFrame, int nId, int nUser) {
    std::list<TunnelEvent>::iterator it = mEvents.begin();
    while (it != mEvents.end() && !(flFrame <= it->mFrame)) {
        ++it;
    }
    it = mEvents.insert(it, TunnelEvent(pObject, flFrame, nId, nUser));
    if (it->mObject != nullptr) {
        it->mObject->AddRef(this);
    }
}

// 0x0046d540
int Tunnel::MoveEvent(int nId, float flFrame) {
    for (std::list<TunnelEvent>::iterator it = mEvents.begin(); it != mEvents.end(); ++it) {
        if (it->mId == nId) {
            Drawable *pObject = it->mObject;
            mEvents.erase(it);
            // Yes, the reference taken for the original entry is not dropped.
            AddEvent(pObject, flFrame, nId, 0);
            return 1;
        }
    }
    return 0;
}

// 0x0046d5d8
int Tunnel::RemoveEvent(int nId) {
    for (std::list<TunnelEvent>::iterator it = mEvents.begin(); it != mEvents.end(); ++it) {
        if (it->mId == nId) {
            if (it->mObject != nullptr) {
                it->mObject->RemoveRef(this);
            }
            mEvents.erase(it);
            return 1;
        }
    }
    return 0;
}

// 0x0046d680
int Tunnel::RemoveEventsInRange(float flFrom, float flTo) {
    int nRemoved = 0;
    std::list<TunnelEvent>::iterator it = mEvents.begin();
    while (it != mEvents.end()) {
        if (it->mFrame < flTo && flFrom <= it->mFrame) {
            if (it->mObject != nullptr) {
                it->mObject->RemoveRef(this);
            }
            ++nRemoved;
            it = mEvents.erase(it);
        } else {
            ++it;
        }
    }
    return nRemoved;
}

// 0x00477310
void Tunnel::ForEachEvent(void (*pfnVisit)(Drawable *pObject, float flFrame, int nId, void *pUser),
                          void *pUser) {
    for (const TunnelEvent &event : mEvents) {
        pfnVisit(event.mObject, event.mFrame, event.mId, pUser);
    }
}

// 0x00476288
Tunnel *NewTunnel(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Tunnel" and the object is 0x104 bytes.
    return new Tunnel(name);
}

} // namespace Rnd
