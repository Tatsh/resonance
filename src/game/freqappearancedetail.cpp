#include "game/freqappearancedetail.h"

#include <cstring>
#include <map>

#include "game/freqparttemplate.h"
#include "math/vector3.h"
#include "met/metfreqmakerassetmanager.h"
#include "os/hxstr.h"
#include "rnd/mesh.h"
#include "rnd/view.h"

namespace {

// The local transform row that stores the translation.
constexpr int kXfmRowTranslation = 3;

// The value the scale and its steps hold before a template is placed.
constexpr float kUnsetScale = 1000.0f;
constexpr int kUnsetScaleStep = -1;

// unpack() scales each placed mesh by this factor and records the scale in steps of its inverse.
constexpr float kPlacedScaleFactor = 110.0f;
// unpack() raises each placed part to this height.
constexpr float kPlacedHeight = 0.1f;

// The flag unpack() compares FreqPart::mMirrored against.
constexpr int kMirrored = 1;

// The placement the preview mesh is parked at by resetCursor().
const Vector3 kCursorOrigin = {0.0f, 0.0f, 0.0f, 1.0f};

// Write a position into the translation row of a mesh and mark the mesh dirty.
inline void PlaceMesh(Rnd::Mesh *pMesh, const Vector3 &position) {
    std::memcpy(pMesh->mLocalXfm[kXfmRowTranslation], &position, sizeof(position));
    pMesh->mDirty = 1;
}

} // namespace

// 0x00249c40
FreqAppearanceDetail::FreqAppearanceDetail() {
    mView = new Rnd::View(MetFreqMakerAssetManager::shared()->NextMeshName());
    mView->SetShowing(1);
    mParts.resize(0); // Yes, the binary resizes the empty list to zero.
}

// 0x00249e58
FreqAppearanceDetail::FreqAppearanceDetail(const FreqAppearanceDetail &other) {
    mView = new Rnd::View(MetFreqMakerAssetManager::shared()->NextMeshName());
    mView->SetShowing(1);
    mParts.resize(0); // Yes, the binary resizes the empty list to zero.
    copyFrom(other);
}

// 0x0024edc8
FreqAppearanceDetail::~FreqAppearanceDetail() {
    clear();
    delete mView;
    mView = nullptr;
}

// 0x0024a4b0
void FreqAppearanceDetail::clear() {
    int nCount = mParts.size();
    for (int i = 0; i < nCount; ++i) {
        FreqPart *pPart = mParts.front();
        mParts.pop_front();
        delete pPart;
    }
    mParts.clear();
    mParts.resize(0); // Yes, the binary resizes the emptied list to zero.

    mView->ClearDraws();
    mView->ClearTransList();
    resetCursor();
    mView->SetShowing(1);
}

// 0x0024a088
void FreqAppearanceDetail::copyFrom(const FreqAppearanceDetail &other) {
    std::map<Rnd::Drawable *, FreqPart *> partsByMesh;
    for (std::list<FreqPart *>::const_iterator it = other.mParts.begin(); it != other.mParts.end();
         ++it) {
        partsByMesh[(*it)->GetMesh()] = *it;
    }

    std::list<Rnd::Drawable *> &draws = other.mView->GetDraws();
    for (std::list<Rnd::Drawable *>::iterator it = draws.begin(); it != draws.end(); ++it) {
        if (*it == static_cast<Rnd::Drawable *>(other.mCursorMesh)) {
            continue;
        }
        FreqPart *pCopy = new FreqPart(*partsByMesh[*it]);
        pCopy->GetMesh()->SetShowing(1);
        mParts.push_back(pCopy);
        mView->AddDraw(pCopy->GetMesh(), nullptr);
        mView->AddTrans(pCopy->GetMesh());
    }
}

// 0x0024b898
void FreqAppearanceDetail::unpack(const FreqPart::Packed *pRecords, int nCount) {
    MetFreqMakerAssetManager::shared()->PollLoad(); // Yes, the binary discards the result.
    for (int i = 0; i < nCount; ++i) {
        FreqPart *pPart = new FreqPart();
        pPart->Unpack(pRecords[i]);

        Rnd::Mesh *pMesh = MetFreqMakerAssetManager::shared()->CloneMesh(
            MetFreqMakerAssetManager::shared()->NextMeshName());
        pMesh->SetMaterial(pPart->mTemplate->mMaterial);
        MetFreqMakerAssetManager::shared()->ApplyPartScale(
            pMesh, pPart->mTemplate, &mScaleX, &mScaleZ, kPlacedScaleFactor, kPlacedScaleFactor);
        pPart->mPosition.y = kPlacedHeight;
        PlaceMesh(pMesh, pPart->mPosition);
        mView->AddDraw(pMesh, nullptr);
        mView->AddTrans(pMesh);
        pMesh->SetShowing(1);
        if (pPart->mMirrored == kMirrored) {
            pMesh->MirrorX();
        }

        pPart->SetMesh(pMesh);
        pPart->SetColor(*MetFreqMakerAssetManager::shared()->ColorAt(pPart->mPalettePosition));
        pPart->GetColor(); // Yes, the binary discards this call's result.

        mUnknown78 = 1;
        mScaleStepX = static_cast<int>(mScaleX * kPlacedScaleFactor);
        mScaleStepZ = static_cast<int>(mScaleZ * kPlacedScaleFactor);
        mParts.push_back(pPart);
        resetCursor();
    }
}

// 0x0024c398
void FreqAppearanceDetail::pack(FreqPart::Packed *pOut, int *pCount) {
    int nCount = 0;
    for (std::list<FreqPart *>::iterator it = mParts.begin(); it != mParts.end(); ++it) {
        (*it)->Pack(pOut);
        ++pOut;
        ++nCount;
    }
    (void)mParts.size(); // Yes, the binary counts the list again and discards the count.
    *pCount = nCount;
}

// 0x00249b48
void FreqAppearanceDetail::ensureCursorMesh() {
    if (mCursorMesh == nullptr) {
        MetFreqMakerAssetManager::shared()->PollLoad(); // Yes, the binary discards the result.
        mCursorMesh = MetFreqMakerAssetManager::shared()->CloneMesh(
            MetFreqMakerAssetManager::shared()->NextMeshName());
        mView->AddDraw(mCursorMesh, nullptr);
        mView->AddTrans(mCursorMesh);
    }
    mUnknown04 = 0;
    mUnknown8c = 0;
    mUnknown00 = 0;
}

// 0x0024ee60
void FreqAppearanceDetail::resetCursor() {
    ensureCursorMesh();
    mCursorMesh->SetShowing(0);
    PlaceMesh(mCursorMesh, kCursorOrigin);
    mCursorMesh->SetMaterial(nullptr);

    mScaleStepZ = kUnsetScaleStep;
    mScaleZ = kUnsetScale;
    mUnknown20 = 0;
    mTemplate = nullptr;
    mUnknown00 = 0;
    mUnknown04 = 0;
    mScaleX = kUnsetScale;
    mScaleStepX = kUnsetScaleStep;
    mUnknown8c = 0;
}
