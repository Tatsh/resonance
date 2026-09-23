#include "app/durgemrowstring.h"

#include "app/durgemtrails.h"
#include "app/tunnelcache.h"
#include "math/color.h"
#include "math/plane.h"
#include "math/transform.h"
#include "math/vector3.h"
#include "rnd/string.h"
#include "rnd/tunnel.h"
#include "rnd/view.h"

namespace {

// Tunnel frames per slice row.
constexpr float kSliceFrames = 1920.0f;

// Tangent scale every lane position is taken with.
constexpr float kTangentScale = 0.96f;

// mRow before the first segment and after Clear().
constexpr int kUnplacedRow = -100;
constexpr int kClearedRow = -1000;

// Ribbon points per straight segment.
constexpr int kPointsPerLine = 2;

inline void SetPaddingWords(Transform &xfm) {
    xfm.mBasisX.w = 1.0f;
    xfm.mBasisY.w = 1.0f;
    xfm.mBasisZ.w = 1.0f;
    xfm.mTranslation.w = 1.0f;
}

// out.xyz = a.xyz * flA + b.xyz * flB, with out.w taken from a. A VU0 multiply and accumulate in
// the image.
inline void BlendVector(const Vector3 &a, float flA, const Vector3 &b, float flB, Vector3 &out) {
    out.x = a.x * flA + b.x * flB;
    out.y = a.y * flA + b.y * flB;
    out.z = a.z * flA + b.z * flB;
    out.w = a.w;
}

} // namespace

// 0x00432830
DurGemRowString::DurGemRowString(int nLane, Rnd::Mat *pMat, Rnd::View *pView, float flWidth)
    : mString(nullptr), mLane(nLane), mRow(kUnplacedRow), mCount(0) {
    mString = new Rnd::String(DurGemTrails::NewStringName());
    mString->SetLinePairs(1);
    mString->SetMat(pMat);
    mString->SetWidth(flWidth);
    pView->AddDraw(mString);
}

// 0x00436ee0
DurGemRowString::~DurGemRowString() {
    delete mString;
}

// 0x00436f48
void DurGemRowString::AddLine(const Color &color,
                              float flStartFrame,
                              float flStartBlend,
                              float flEndFrame,
                              float flEndBlend) {
    const int nRow = static_cast<int>(flStartFrame / kSliceFrames);
    if (mRow != nRow) {
        mRow = nRow;
        mCount = 0;
    }
    const int nFirst = mCount * kPointsPerLine;
    ++mCount;
    mString->SetNumPoints(mCount * kPointsPerLine);

    Transform xfm;
    SetPaddingWords(xfm);
    GetCachedTunnelObject()->ProjectSectionToCameraSpace(
        mLane, &xfm, flStartFrame, flStartBlend, kTangentScale);
    mString->SetPointColor(nFirst, color);
    mString->SetPointPos(nFirst, xfm.mTranslation);
    GetCachedTunnelObject()->ProjectSectionToCameraSpace(
        mLane, &xfm, flEndFrame, flEndBlend, kTangentScale);
    mString->SetPointColor(nFirst + 1, color);
    mString->SetPointPos(nFirst + 1, xfm.mTranslation);
}

// 0x004370b8
void DurGemRowString::Clear() {
    mRow = kClearedRow;
    mCount = 0;
    mString->SetShowing(0);
}

// 0x004370f8
int DurGemRowString::Show(int nRow) {
    if (nRow == mRow) {
        mString->SetShowing(1);
        return mString->GetNumPoints();
    }
    mString->SetShowing(0);
    return 0;
}

// 0x00437180
void DurGemRowString::Hide() {
    mString->SetShowing(0);
}

// 0x004371b0
bool DurGemRowString::FindCrossing(const Plane &plane, Vector3 *pOut) {
    for (int i = 0; i < mCount; ++i) {
        const Vector3 segment[] = {
            *mString->GetPointPos(i * kPointsPerLine),
            *mString->GetPointPos(i * kPointsPerLine + 1),
        };
        float flT;
        IntersectSegmentWithPlane(segment, plane, &flT); // Yes, the binary discards this result.
        if (0.0f <= flT && flT <= 1.0f) {
            BlendVector(segment[1], flT, segment[0], 1.0f - flT, *pOut);
            return true;
        }
    }
    return false;
}
