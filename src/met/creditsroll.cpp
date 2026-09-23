#include "met/creditsroll.h"

#include <stdio.h>

#include "game/freqappearance.h"
#include "math/vector2.h"
#include "math/vector3.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metpersonadata.h"
#include "rnd/cam.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/meshvert.h"
#include "rnd/text.h"
#include "rnd/transformable.h"

namespace {

// The numbered object names, the prefix followed by a three-digit credit number.
constexpr char kNameFormat[] = "%s%3.3i";

// The frame reserves this much for a formatted name.
constexpr int kNameBufferSize = 112;

// The row of a world transform that holds the translation.
constexpr int kXfmRowTranslation = 3;

// The picture material's stage that shows the persona burn texture.
constexpr int kBurnStage = 1;

// The unit square's edges on the vertical axis.
constexpr float kUnitTop = 0.0f;
constexpr float kUnitBottom = 1.0f;

// A point carried through a world transform, with out.w taken from the input. A VU0 multiply and
// accumulate in the image.
inline void XfmPointByWorld(const Vector3 &in,
                            const float (&xfm)[Rnd::kXfmRowCount][Rnd::kXfmRowFloatCount],
                            Vector3 &out) {
    const float flX = xfm[0][0] * in.x + xfm[1][0] * in.y + xfm[2][0] * in.z + xfm[3][0];
    const float flY = xfm[0][1] * in.x + xfm[1][1] * in.y + xfm[2][1] * in.z + xfm[3][1];
    const float flZ = xfm[0][2] * in.x + xfm[1][2] * in.y + xfm[2][2] * in.z + xfm[3][2];
    out.x = flX;
    out.y = flY;
    out.z = flZ;
    out.w = in.w;
}

inline const char *NameOrEmpty(const HxStr &name) {
    return name.mStr != nullptr ? name.mStr : g_szEmptyString;
}

} // namespace

// 0x00164df8
CreditsRoll::CreditsRoll(const HxStr &picturePrefix,
                         const HxStr &textPrefix,
                         Rnd::Cam *pCam,
                         int nFirstIndex)
    : picturePrefix_(picturePrefix), textPrefix_(textPrefix), cam_(pCam), firstVisible_(0),
      firstIndex_(nFirstIndex), identities_(nullptr), burnSlot_(0) {
    Reset();
}

// 0x00165110
void CreditsRoll::Reset() {
    burnSlot_ = 0;
    firstVisible_ = firstIndex_;
    identities_ = MetFreqMakerAssetManager::shared()->GetAllIdentities();
    const int nCount = burned_.size();
    for (int i = 0; i < nCount; ++i) {
        burned_[i] = false;
    }
}

// 0x00165268
int CreditsRoll::Update() {
    for (int nIndex = firstVisible_;; ++nIndex) {
        Rnd::Mesh *pPicture = GetPicture(nIndex);
        Rnd::Text *pText = GetText(nIndex);
        if (pPicture == nullptr && pText == nullptr) {
            return 0;
        }

        switch (Classify(pPicture, pText)) {
        case kBandVisible:
            if (static_cast<unsigned>(nIndex) < personaNames_.size()) {
                const HxStr name(personaNames_[nIndex]);
                if (name.mLen != 0 && !burned_[nIndex]) {
                    // Yes, the binary attaches through a null persona when the name is not found.
                    FindPersona(name, *identities_)->AttachToBurnSlot(burnSlot_);
                    pPicture->mMat->mStages[kBurnStage].SetTex(
                        FreqAppearance::FindPersonaBurnTexture(burnSlot_));
                    burned_[nIndex] = true;
                    if (++burnSlot_ >= kBurnSlotCount) {
                        burnSlot_ = 0;
                    }
                }
            }
            SetShowing(pPicture, pText, 1);
            break;
        case kBandAbove:
            SetShowing(pPicture, pText, 0);
            firstVisible_ = nIndex + 1;
            break;
        case kBandBelow:
            SetShowing(pPicture, pText, 0);
            return 1;
        default:
            break;
        }
    }
}

// 0x00165658
int CreditsRoll::ClassifyPicture(Rnd::Mesh *pPicture) {
    if (pPicture == nullptr) {
        return kBandAbsent;
    }

    const std::vector<Rnd::MeshVert> &verts = pPicture->mVertsOwner->mVerts;
    const int nVerts = verts.size();
    int nBands = 0;
    Vector3 world;
    for (int i = 0; i < nVerts; ++i) {
        XfmPointByWorld(verts[i].mPoint, pPicture->mWorldXfm, world);
        const Vector2 unit = cam_->ProjectToUnit(world);
        if (unit.y < kUnitTop) {
            nBands |= kBandAbove;
        } else if (kUnitBottom < unit.y) {
            nBands |= kBandBelow;
        } else {
            nBands |= kBandVisible;
        }
    }

    // A picture that spans the whole band counts as visible.
    if ((nBands & kBandVisible) != 0 ||
        (nBands & (kBandAbove | kBandBelow)) == (kBandAbove | kBandBelow)) {
        return kBandVisible;
    }
    if ((nBands & kBandAbove) != 0) {
        return kBandAbove;
    }
    return (nBands & kBandBelow) != 0 ? kBandBelow : kBandAbsent;
}

// 0x001658c8
int CreditsRoll::ClassifyText(Rnd::Text *pText) {
    if (pText == nullptr) {
        return kBandAbsent;
    }

    float flTop;
    float flBottom;
    pText->GetVerticalBounds(flTop, flBottom);
    const float *pTranslation = pText->mWorldXfm[kXfmRowTranslation];
    Vector3 top{pTranslation[0], pTranslation[1], pTranslation[2], pTranslation[3]};
    top.z += flTop;
    Vector3 bottom{pTranslation[0], pTranslation[1], pTranslation[2], pTranslation[3]};
    bottom.z += flBottom;

    const Vector2 unitTop = cam_->ProjectToUnit(top);
    const Vector2 unitBottom = cam_->ProjectToUnit(bottom);
    if (unitBottom.y < kUnitTop) {
        return kBandAbove;
    }
    if (kUnitBottom < unitTop.y) {
        return kBandBelow;
    }
    return kBandVisible;
}

// 0x00165b58
void CreditsRoll::HideAll() {
    const int nPictures = pictures_.size();
    for (int i = 0; i < nPictures; ++i) {
        if (pictures_[i] != nullptr) {
            pictures_[i]->SetShowing(0);
        }
    }
    const int nTexts = texts_.size();
    for (int i = 0; i < nTexts; ++i) {
        if (texts_[i] != nullptr) {
            texts_[i]->SetShowing(0);
        }
    }
}

// 0x00165c38
void CreditsRoll::Build() {
    int nCount = 0;
    for (int nIndex = firstIndex_;; ++nIndex, ++nCount) {
        char szName[kNameBufferSize];
        sprintf(szName, kNameFormat, NameOrEmpty(picturePrefix_), nIndex);
        Rnd::Mesh *pPicture = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(szName)));
        sprintf(szName, kNameFormat, NameOrEmpty(textPrefix_), nIndex);
        Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(szName)));
        if (pPicture == nullptr && pText == nullptr) {
            break;
        }
        pictures_.push_back(pPicture);
        texts_.push_back(pText);
    }

    burned_.resize(nCount);
    personaNames_.resize(nCount);

    // Each index is a credit number. Yes, the binary writes them without checking them against
    // the credit count.
    personaNames_[5] = "Greg";
    personaNames_[6] = "Alex";
    personaNames_[7] = "Eran";
    personaNames_[8] = "robotkid";
    personaNames_[9] = "Ryan";
    personaNames_[10] = "Tracy";
    personaNames_[12] = "Dfan";
    personaNames_[13] = "Eric";
    personaNames_[14] = "Christine";
    personaNames_[15] = "Doug";
    personaNames_[16] = "M C J";
    personaNames_[17] = "Rex";
    personaNames_[18] = "Jon";
    personaNames_[19] = "Denny";
    personaNames_[23] = "Kasson";
    personaNames_[24] = "Tony";
    personaNames_[25] = "Chris";
    personaNames_[26] = "Juno";
    personaNames_[29] = "Dare";
    personaNames_[30] = "J  A";
    personaNames_[31] = "Adolph";
    personaNames_[34] = "Erik";
    personaNames_[35] = "Jeremy";
    personaNames_[36] = "DeVron";
    personaNames_[37] = "Daniel";
    personaNames_[40] = "Mike";
    personaNames_[41] = "Kris";
    personaNames_[42] = "Warburg";

    HideAll();
}

// 0x00169860
MetPersonaData *CreditsRoll::FindPersona(const HxStr &name,
                                         const std::vector<MetPersonaData *> &identities) {
    const int nIdentities = identities.size();
    for (int i = 0; i < nIdentities; ++i) {
        if (name == identities[i]->mUnknown140.mUnknown00) {
            return identities[i];
        }
    }
    return nullptr;
}

// 0x00169900
Rnd::Mesh *CreditsRoll::GetPicture(int nIndex) {
    const unsigned nOffset = nIndex - firstIndex_;
    return nOffset < pictures_.size() ? pictures_[nOffset] : nullptr;
}

// 0x00169938
Rnd::Text *CreditsRoll::GetText(int nIndex) {
    const unsigned nOffset = nIndex - firstIndex_;
    return nOffset < texts_.size() ? texts_[nOffset] : nullptr;
}

// 0x00169970
int CreditsRoll::Classify(Rnd::Mesh *pPicture, Rnd::Text *pText) {
    const int nPicture = ClassifyPicture(pPicture);
    const int nText = ClassifyText(pText);
    if (nPicture == kBandVisible || nText == kBandVisible) {
        return kBandVisible;
    }
    return nPicture == kBandAbsent ? nText : nPicture;
}

// 0x001699e8
void CreditsRoll::SetShowing(Rnd::Mesh *pPicture, Rnd::Text *pText, int nShowing) {
    if (pPicture != nullptr) {
        pPicture->SetShowing(nShowing);
    }
    if (pText != nullptr) {
        pText->SetShowing(nShowing);
    }
}
