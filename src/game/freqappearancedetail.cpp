#include "game/freqappearancedetail.h"

#include <cstring>
#include <map>

#include "game/freqparttemplate.h"
#include "math/vector3.h"
#include "met/metfreqmakerassetmanager.h"
#include "os/hxstr.h"
#include "os/r250.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/tex.h"
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

// The literal save() compares each template name against.
static const char *const kNoName = "";

// An avatar has at most this many parts.
constexpr unsigned kMaxParts = 16;

// The flags placeCursor() copies the preview mesh with.
constexpr unsigned kMeshCopyFlags = 0;

// nudgeCursor() keeps each edge of the part, half its scale steps from the cursor and inset by a
// tenth of them, inside these bounds.
constexpr double kHalf = 0.5;
constexpr float kEdgeInset = 0.1f;
constexpr int kLowBound = -55;
constexpr int kHighBound = 55;

// A part of this category cannot rise above the zero line, and randomize() does not swap its
// template. randomize() does not swap the templates of the other two categories.
constexpr int kGroundedCategory = 1;
constexpr int kNoSwapCategory2 = 2;
constexpr int kNoSwapCategory8 = 8;

// The rolls below which randomize() recolours a part, and then swaps its template.
constexpr float kRecolorChance = 0.33333f;
constexpr float kReshapeChance = 0.66666f;

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
        addLoadedPart(pPart);
    }
}

// 0x0024b160
void FreqAppearanceDetail::save(OBStream &stream) {
    int nCount = mParts.size();
    stream.Write(&nCount, sizeof(nCount));
    for (std::list<FreqPart *>::iterator it = mParts.begin(); it != mParts.end(); ++it) {
        FreqPart *pPart = *it;
        pPart->GetColor();                          // Yes, the binary discards this call's result.
        (void)(pPart->mTemplate->mName != kNoName); // Yes, the binary discards this test.

        const HxStr &name = pPart->mTemplate->mName;
        unsigned nLength = name.mLen;
        stream.Write(&nLength, sizeof(nLength));
        OBStream &out =
            stream.WriteBytes(name.mStr != nullptr ? name.mStr : g_szEmptyString, nLength);
        float flPaletteX = pPart->mPalettePosition.x;
        float flPaletteY = pPart->mPalettePosition.y;
        float flX = pPart->mPosition.x;
        float flZ = pPart->mPosition.z;
        (out.Write(&flPaletteX, sizeof(flPaletteX)).Write(&flPaletteY, sizeof(flPaletteY))
         << pPart->mMirrored)
            .Write(&flX, sizeof(flX))
            .Write(&flZ, sizeof(flZ));
    }
}

// 0x0024b340
void FreqAppearanceDetail::load(IBStream &stream) {
    MetFreqMakerAssetManager::shared()->PollLoad(); // Yes, the binary discards the result.
    int nCount;
    stream.Read(&nCount, sizeof(nCount));
    for (int i = 0; i < nCount; ++i) {
        HxStr name;
        unsigned nLength;
        stream.Read(&nLength, sizeof(nLength));
        name.Alloc(nLength);
        stream.ReadBytes(name.mStr != nullptr ? name.mStr : const_cast<char *>(g_szEmptyString),
                         nLength);
        // Yes, the binary looks the name up as a texture and discards the result.
        (void)dynamic_cast<Rnd::Tex *>(Rnd::g_manager.Find(name));

        FreqPart *pPart = new FreqPart(MetFreqMakerAssetManager::shared()->FindPart(name));
        stream.Read(&pPart->mPalettePosition.x, sizeof(pPart->mPalettePosition.x));
        stream.Read(&pPart->mPalettePosition.y, sizeof(pPart->mPalettePosition.y));
        stream >> pPart->mMirrored;
        stream.Read(&pPart->mPosition.x, sizeof(pPart->mPosition.x));
        stream.Read(&pPart->mPosition.z, sizeof(pPart->mPosition.z));
        addLoadedPart(pPart);
    }
}

inline void FreqAppearanceDetail::addLoadedPart(FreqPart *pPart) {
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

    mPlacing = 1;
    mScaleStepX = static_cast<int>(mScaleX * kPlacedScaleFactor);
    mScaleStepZ = static_cast<int>(mScaleZ * kPlacedScaleFactor);
    mParts.push_back(pPart);
    resetCursor();
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
    mCursorZ = 0;
    mCursorMirrored = 0;
    mCursorX = 0;
}

// 0x0024ee60
void FreqAppearanceDetail::resetCursor() {
    ensureCursorMesh();
    mCursorMesh->SetShowing(0);
    PlaceMesh(mCursorMesh, kCursorOrigin);
    mCursorMesh->SetMaterial(nullptr);

    mScaleStepZ = kUnsetScaleStep;
    mScaleZ = kUnsetScale;
    mSelected = nullptr;
    mTemplate = nullptr;
    mCursorX = 0;
    mCursorZ = 0;
    mScaleX = kUnsetScale;
    mScaleStepX = kUnsetScaleStep;
    mCursorMirrored = 0;
}

// 0x0024a5f0
void FreqAppearanceDetail::selectTemplate(const HxStr &name) {
    ensureCursorMesh();
    mSelected = nullptr;
    mPlacing = 1;
    resetCursor();
    if (mParts.size() >= kMaxParts) {
        return;
    }

    mTemplate = MetFreqMakerAssetManager::shared()->FindPart(name);
    mCursorMesh->SetMaterial(mTemplate->mMaterial);
    MetFreqMakerAssetManager::shared()->ApplyPartScale(
        mCursorMesh, mTemplate, &mScaleX, &mScaleZ, kPlacedScaleFactor, kPlacedScaleFactor);
    setColor(mColor, mPalettePosition);
    mScaleStepX = static_cast<int>(mScaleX * kPlacedScaleFactor);
    mScaleStepZ = static_cast<int>(mScaleZ * kPlacedScaleFactor);
    mCursorMesh->SetShowing(1);

    // Hanging the preview mesh again moves it to the end of the draw and transform lists.
    mView->RemoveDraw(mCursorMesh);
    mView->RemoveTrans(mCursorMesh);
    mView->AddDraw(mCursorMesh, nullptr);
    mView->AddTrans(mCursorMesh);
}

// 0x0024a770
void FreqAppearanceDetail::placeCursor() {
    if (mPlacing == 1) {
        if (mTemplate == nullptr) {
            return;
        }
        Rnd::Mesh *pMesh = MetFreqMakerAssetManager::shared()->CloneMesh(
            MetFreqMakerAssetManager::shared()->NextMeshName());
        pMesh->Copy(mCursorMesh, kMeshCopyFlags);
        pMesh->SetMaterial(mTemplate->mMaterial);
        mView->AddDraw(pMesh, mCursorMesh);
        mView->AddTrans(pMesh);
        mView->MoveDraw(mCursorMesh, 1);

        FreqPart *pPart = new FreqPart(mTemplate);
        pPart->mMirrored = mCursorMirrored;
        pPart->mPosition.x = mCursorX;
        pPart->mPosition.z = mCursorZ;
        pPart->mPosition.y = 0.0f;
        pPart->SetMesh(pMesh);
        pPart->mPalettePosition = mPalettePosition;
        pPart->SetColor(mColor);
        mParts.push_back(pPart);
        resetCursor();
    } else if (mPlacing == 0) {
        deselect();
        mPlacing = 1;
    }
}

// 0x0024a9e8
void FreqAppearanceDetail::nudgeCursor(int nStepX, int nStepZ) {
    if (mParts.size() >= kMaxParts && mPlacing == 1) {
        return;
    }
    if (mPlacing == 0 && (mSelected == nullptr || mSelected->mTemplate == nullptr)) {
        return;
    }

    Rnd::Mesh *pMesh = nullptr;
    FreqPartTemplate *pTemplate = nullptr;
    if (mPlacing == 1) {
        pTemplate = mTemplate;
        if (pTemplate == nullptr) {
            return;
        }
        pMesh = mCursorMesh;
    } else if (mPlacing == 0) {
        if (mSelected == nullptr) {
            return;
        }
        pMesh = mSelected->GetMesh();
        pTemplate = mSelected->mTemplate;
    }

    // The bounds arithmetic runs in double precision through the software floating-point library.
    int nEdgeHighX = static_cast<int>(mCursorX + mScaleStepX * kHalf -
                                      static_cast<double>(mScaleStepX * kEdgeInset));
    int nEdgeLowX = static_cast<int>(mCursorX - mScaleStepX * kHalf +
                                     static_cast<double>(mScaleStepX * kEdgeInset));
    if (nEdgeHighX > kLowBound && nStepX == -1) {
        --mCursorX;
    } else if (nEdgeLowX < kHighBound && nStepX == 1) {
        ++mCursorX;
    }

    int nEdgeLowZ = static_cast<int>(mCursorZ - mScaleStepZ * kHalf +
                                     static_cast<double>(mScaleStepZ * kEdgeInset));
    int nEdgeHighZ = static_cast<int>(mCursorZ + mScaleStepZ * kHalf -
                                      static_cast<double>(mScaleStepZ * kEdgeInset));
    if (nEdgeLowZ < kHighBound && nStepZ == 1) {
        bool bCanMove;
        if (pTemplate->mCategory != kGroundedCategory) {
            bCanMove = true;
        } else {
            pTemplate->mCategory = nStepZ; // Yes, the binary stores the step, 1 here, back.
            bCanMove = mCursorZ < 0;
        }
        if (bCanMove) {
            ++mCursorZ;
        }
    } else if (nEdgeHighZ > kLowBound && nStepZ == -1) {
        --mCursorZ;
    }

    Vector3 position = {
        static_cast<float>(mCursorX), kPlacedHeight, static_cast<float>(mCursorZ), 1.0f};
    PlaceMesh(pMesh, position);
    if (mPlacing == 0) {
        mSelected->mPosition.x = mCursorX;
        mSelected->mPosition.z = mCursorZ;
    }
}

// 0x0024ae20
FreqPart *FreqAppearanceDetail::selectPart(int nIndex) {
    ensureCursorMesh();
    mPlacing = 0;
    resetCursor();
    if (nIndex < 0 || nIndex >= static_cast<int>(mParts.size())) {
        resetCursor();
        return nullptr;
    }

    FreqPart *pPart = partAt(nIndex);
    if (pPart == nullptr) {
        return nullptr;
    }
    mSelected = pPart;
    mSelectedBackup = *mSelected;
    mSelectedDrawIndex = drawIndexOf(mSelected);

    MetFreqMakerAssetManager::shared()->ApplyPartScale(mSelected->GetMesh(),
                                                       mSelected->mTemplate,
                                                       &mScaleX,
                                                       &mScaleZ,
                                                       kPlacedScaleFactor,
                                                       kPlacedScaleFactor);
    // ApplyPartScale() rebuilds the basis, which drops the mirroring.
    if (mSelected->mMirrored == kMirrored) {
        mSelected->GetMesh()->MirrorX();
    }
    mScaleStepX = static_cast<int>(mScaleX * kPlacedScaleFactor);
    mScaleStepZ = static_cast<int>(mScaleZ * kPlacedScaleFactor);
    mCursorX = static_cast<int>(mSelected->mPosition.x);
    mCursorZ = static_cast<int>(mSelected->mPosition.z);
    return mSelected;
}

// 0x0024bc50
void FreqAppearanceDetail::deletePart(int nIndex) {
    ensureCursorMesh();
    resetCursor();
    if (nIndex < 0 || nIndex >= static_cast<int>(mParts.size())) {
        return;
    }

    FreqPart *pPart = partAt(nIndex);
    if (pPart == nullptr) {
        return;
    }
    mParts.remove(pPart);
    mView->RemoveDraw(pPart->GetMesh());
    mView->RemoveTrans(pPart->GetMesh());
    delete pPart;
}

// 0x0024be00
void FreqAppearanceDetail::revertSelection() {
    if (mSelected == nullptr) {
        return;
    }

    mSelected->SetColor(*mSelectedBackup.GetColor());
    if (mSelected->mMirrored != mSelectedBackup.mMirrored) {
        mSelected->GetMesh()->MirrorX();
        mSelected->mMirrored ^= 1;
    }
    mView->MoveDraw(mSelected->GetMesh(), mSelectedDrawIndex - drawIndexOf(mSelected));

    mSelected->mPosition = mSelectedBackup.mPosition;
    mCursorX = static_cast<int>(mSelected->mPosition.x);
    mCursorZ = static_cast<int>(mSelected->mPosition.z);
    Vector3 position = {
        static_cast<float>(mCursorX), kPlacedHeight, static_cast<float>(mCursorZ), 1.0f};
    PlaceMesh(mSelected->GetMesh(), position);
    mSelected = nullptr;
}

// 0x0024bfc8
void FreqAppearanceDetail::randomize() {
    MetFreqMakerAssetManager::shared()->PollLoad(); // Yes, the binary discards the result.
    for (std::list<FreqPart *>::iterator it = mParts.begin(); it != mParts.end(); ++it) {
        FreqPart *pPart = *it;
        if (pPart->mTemplate->mRandomizable == 0) {
            continue;
        }

        float flRoll = RandomFloat();
        if (flRoll < kRecolorChance) {
            pPart->mPalettePosition.x = RandomFloat();
            pPart->mPalettePosition.y = RandomFloat();
            pPart->SetColor(*MetFreqMakerAssetManager::shared()->ColorAt(pPart->mPalettePosition));
        } else if (flRoll < kReshapeChance) {
            int nCategory = pPart->mTemplate->mCategory;
            if (nCategory == kGroundedCategory || nCategory == kNoSwapCategory2 ||
                nCategory == kNoSwapCategory8) {
                continue;
            }
            std::list<FreqPartTemplate *> *pTemplates =
                MetFreqMakerAssetManager::shared()->TemplatesInCategory(nCategory);
            int nCount = pTemplates->size();
            if (nCount == 0) {
                continue;
            }

            int nPick = static_cast<int>(RandomFloat() * (nCount - 1.0f));
            std::list<FreqPartTemplate *>::iterator pick = pTemplates->begin();
            for (int i = 0; pick != pTemplates->end() && i != nPick; ++i) {
                ++pick;
            }
            pPart->mTemplate = *pick;
            pPart->GetMesh()->SetMaterial(pPart->mTemplate->mMaterial);
            MetFreqMakerAssetManager::shared()->ApplyPartScale(pPart->GetMesh(),
                                                               pPart->mTemplate,
                                                               &mScaleX,
                                                               &mScaleZ,
                                                               kPlacedScaleFactor,
                                                               kPlacedScaleFactor);
        }
    }
}

// 0x0024c218
void FreqAppearanceDetail::recentrePart(int nIndex) {
    if (nIndex < 0 || nIndex >= static_cast<int>(mParts.size())) {
        return;
    }
    FreqPart *pPart = partAt(nIndex);
    if (pPart == nullptr) {
        return;
    }

    Vector3 position = {0.0f, kPlacedHeight, 0.0f, 1.0f};
    PlaceMesh(pPart->GetMesh(), position);
    pPart->mPosition.x = 0.0f;
    pPart->mPosition.z = 0.0f;
    mCursorX = 0;
    mCursorZ = 0;

    // Moving the mesh one step forward once per part brings it to the front.
    Rnd::Mesh *pMesh = pPart->GetMesh();
    for (int nSteps = mParts.size(); nSteps > 0; --nSteps) {
        mView->MoveDraw(pMesh, 1);
    }
}

// 0x0024ef08
void FreqAppearanceDetail::sendBackward() {
    moveCursorDraw(-1);
}

// 0x0024ef80
void FreqAppearanceDetail::bringForward() {
    moveCursorDraw(1);
}

// 0x0024eff8
int FreqAppearanceDetail::drawIndexOf(FreqPart *pPart) {
    Rnd::Mesh *pMesh = pPart->GetMesh();
    std::list<Rnd::Drawable *> &draws = mView->GetDraws();
    int nIndex = 0;
    for (std::list<Rnd::Drawable *>::iterator it = draws.begin(); it != draws.end(); ++it) {
        if (*it == static_cast<Rnd::Drawable *>(pMesh)) {
            break;
        }
        ++nIndex;
    }
    return nIndex;
}

// 0x0024f060
void FreqAppearanceDetail::toggleMirror() {
    if (mPlacing == 1) {
        mCursorMirrored ^= 1;
        mCursorMesh->MirrorX();
    } else if (mPlacing == 0) {
        mSelected->GetMesh()->MirrorX();
        mSelected->mMirrored ^= 1;
    }
}

// 0x0024f178
void FreqAppearanceDetail::setColor(const Color &color, const Vector2 &palettePosition) {
    mColor = color;
    mPalettePosition = palettePosition;
    if (mPlacing == 1) {
        mCursorMesh->SetVertexColor(mColor);
    } else if (mPlacing == 0 && mSelected != nullptr) {
        mSelected->SetColor(color);
        mSelected->mPalettePosition = palettePosition;
        mSelected->GetMesh()->SetVertexColor(color);
    }
}

// 0x0024f238
void FreqAppearanceDetail::attachTo(Rnd::View *pParent) {
    pParent->AddDraw(mView, nullptr);
    pParent->AddTrans(mView);
}

// 0x0024f290
void FreqAppearanceDetail::detachFrom(Rnd::View *pParent) {
    pParent->RemoveDraw(mView);
    pParent->RemoveTrans(mView);
}

inline FreqPart *FreqAppearanceDetail::partAt(int nIndex) {
    std::list<FreqPart *>::iterator it = mParts.begin();
    FreqPart *pPart = nullptr;
    if (it != mParts.end()) {
        pPart = *it;
        for (int i = 0; i != nIndex; ++i) {
            if (++it == mParts.end()) {
                break;
            }
            pPart = *it;
        }
    }
    return pPart;
}

inline void FreqAppearanceDetail::moveCursorDraw(int nSteps) {
    Rnd::Drawable *pDraw = nullptr;
    if (mPlacing == 1) {
        if (mCursorMesh == nullptr) {
            return;
        }
        pDraw = mCursorMesh;
    } else if (mPlacing == 0) {
        if (mSelected == nullptr) {
            return;
        }
        pDraw = mSelected->GetMesh();
    }
    mView->MoveDraw(pDraw, nSteps);
}
