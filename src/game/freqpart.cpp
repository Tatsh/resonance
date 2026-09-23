#include "game/freqpart.h"

#include "game/freqparttemplate.h"
#include "met/metfreqmakerassetmanager.h"
#include "os/hxstr.h"
#include "rnd/mesh.h"

namespace {

// A reset part has no palette position.
constexpr float kNoPalettePosition = -1.0f;

// The byte range a palette coordinate is scaled to in the transfer form.
constexpr float kPaletteByteScale = 255.0f;

// The flags operator=() copies the other part's mesh with.
constexpr unsigned kMeshCopyFlags = 0;

// Clamp a palette coordinate to the unit range as Pack() does.
inline float ClampToUnit(float flValue) {
    if (flValue > 1.0f) {
        return 1.0f;
    }
    return flValue < 0.0f ? 0.0f : flValue;
}

} // namespace

// 0x00176f38
FreqPart::FreqPart() {
    mPosition.w = 1.0f;
    mPalettePosition.x = 0.0f;
    mPalettePosition.y = 0.0f;
    Reset();
}

// 0x00176ed0
FreqPart::FreqPart(FreqPartTemplate *pTemplate) {
    mPosition.w = 1.0f;
    mPalettePosition.x = 0.0f;
    mPalettePosition.y = 0.0f;
    Reset();
    // The binary clears the placement again after Reset() has.
    mTemplate = pTemplate;
    mPosition.x = 0.0f;
    mPosition.z = 0.0f;
    mPosition.y = 0.0f;
    mMirrored = 0;
}

// 0x00176f78
FreqPart::FreqPart(const FreqPart &other) {
    mPosition.w = 1.0f;
    mPalettePosition.x = 0.0f;
    mPalettePosition.y = 0.0f;
    Reset();
    *this = other;
}

// 0x00176fd0
FreqPart::~FreqPart() {
    delete mMesh;
}

// 0x00174de8
void FreqPart::operator=(const FreqPart &other) {
    mTemplate = other.mTemplate;
    mPosition = other.mPosition;
    mMirrored = other.mMirrored;
    mColor = other.mColor;
    if (mMesh != nullptr) {
        delete mMesh;
        mMesh = nullptr;
    }
    mMesh = MetFreqMakerAssetManager::shared()->CloneMesh(
        MetFreqMakerAssetManager::shared()->NextMeshName());
    mMesh->Copy(other.mMesh, kMeshCopyFlags);
    mMesh->SetVertexColor(mColor);
    mPalettePosition = other.mPalettePosition;
}

// 0x00177100
void FreqPart::SetMesh(Rnd::Mesh *pMesh) {
    delete mMesh;
    mMesh = pMesh;
}

// 0x00177160
void FreqPart::SetColor(const Color &color) {
    if (mTemplate->mColorable == 0) {
        mColor.r = 0.0f;
        mColor.a = 1.0f;
        mColor.g = 0.0f;
        mColor.b = 0.0f;
    } else {
        mColor = color;
    }
    mMesh->SetVertexColor(mColor);
}

// 0x001771b8
void FreqPart::Reset() {
    mTemplate = nullptr;
    mPosition.x = 0.0f;
    mPosition.z = 0.0f;
    mPosition.y = 0.0f;
    mMirrored = 0;
    mColor = g_freqMakerDefaultColor;
    mPalettePosition.x = kNoPalettePosition;
    mMesh = nullptr; // Reset() does not delete the mesh.
    mPalettePosition.y = kNoPalettePosition;
}

// 0x001771f8
void FreqPart::Pack(Packed *pOut) {
    pOut->mId = mTemplate->mId;
    if (mMirrored != 0) {
        pOut->mId = -mTemplate->mId;
    }
    pOut->mX = static_cast<short>(mPosition.x);
    pOut->mZ = static_cast<short>(mPosition.z);

    mPalettePosition.x = ClampToUnit(mPalettePosition.x);
    pOut->mPaletteX =
        static_cast<unsigned char>(static_cast<int>(mPalettePosition.x * kPaletteByteScale));
    mPalettePosition.y = ClampToUnit(mPalettePosition.y);
    pOut->mPaletteY =
        static_cast<unsigned char>(static_cast<int>(mPalettePosition.y * kPaletteByteScale));
}

// 0x00177038
void FreqPart::Unpack(const Packed &packed) {
    int nId = packed.mId;
    mTemplate = MetFreqMakerAssetManager::shared()->GetPart(nId < 0 ? -nId : nId);
    mMirrored = nId < 0;
    mPosition.y = 0.0f;
    mPosition.x = packed.mX;
    mPosition.z = packed.mZ;
    mPalettePosition.x = packed.mPaletteX / kPaletteByteScale;
    mPalettePosition.y = packed.mPaletteY / kPaletteByteScale;
}
