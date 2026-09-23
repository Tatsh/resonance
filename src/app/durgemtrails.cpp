#include "app/durgemtrails.h"

#include "app/apptunnel.h"
#include "app/durgemrowstring.h"
#include "app/durgemstrip.h"
#include "app/tunnelcache.h"
#include "math/color.h"
#include "math/plane.h"
#include "math/transform.h"
#include "math/vector3.h"
#include "os/formatstring.h"
#include "rnd/manager.h"
#include "rnd/string.h"
#include "rnd/tunnel.h"
#include "rnd/view.h"

namespace {

constexpr int kLaneCount = 8;
constexpr int kRowCount = 8;
constexpr int kStripCount = 6;

// Tunnel frames per slice row, and how far behind the playhead the first shown row starts.
constexpr float kSliceFrames = 1920.0f;
constexpr float kLookBehindFrames = 480.0f;

// Tangent scale every lane position is taken with.
constexpr float kTangentScale = 0.96f;

// Squared distance from the chord past which a midpoint is kept.
constexpr float kMinOffsetSquared = 0.0001f;

// The two segment ends and the middle subdivision point. The middle point remains at the origin
// when the segment is straight.
enum {
    kFirstSegmentPoint = 0,
    kMiddleSegmentPoint = DurGemTrails::kSegmentPointCount / 2,
    kLastSegmentPoint = DurGemTrails::kSegmentPointCount - 1,
};

constexpr const char *kStringNameFormat = "<tnlstr%04d>";

inline void SetPaddingWords(Transform &xfm) {
    xfm.mBasisX.w = 1.0f;
    xfm.mBasisY.w = 1.0f;
    xfm.mBasisZ.w = 1.0f;
    xfm.mTranslation.w = 1.0f;
}

} // namespace

int g_nDurGemStringCount;

// 0x00432f60
DurGemTrails::DurGemTrails(AppTunnel *pTunnel, int nMaxPoints)
    : mLaneCount(kLaneCount), mRowCount(kRowCount), mMaxPoints(nMaxPoints), mTunnel(pTunnel) {
    mRows.resize(mLaneCount * mRowCount, nullptr);
    mView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("tnl strings")));
    mStrips.resize(kStripCount, nullptr);
    for (DurGemStrip *&pStrip : mStrips) {
        pStrip = new DurGemStrip(mView);
    }
}

// 0x00433320
DurGemTrails::~DurGemTrails() {
    for (DurGemRowString *pRow : mRows) {
        delete pRow;
    }
    for (DurGemStrip *pStrip : mStrips) {
        delete pStrip;
    }
}

// 0x004377f0
void DurGemTrails::CreateLane(float flWidth, int nLane, Rnd::Mat *pMat) {
    for (int nRow = 0; nRow < mRowCount; ++nRow) {
        mRows[nRow * mLaneCount + nLane] = new DurGemRowString(nLane, pMat, mView, flWidth);
    }
}

// 0x004337d8
void DurGemTrails::AddSegment(int nLane,
                              const Color &color,
                              float flStartFrame,
                              float flStartBlend,
                              float flEndFrame,
                              float flEndBlend) {
    const int nRow = static_cast<int>(flStartFrame / kSliceFrames);

    Vector3 points[kSegmentPointCount];
    for (Vector3 &point : points) {
        point.x = 0.0f;
        point.y = 0.0f;
        point.z = 0.0f;
        point.w = 1.0f;
    }

    Transform xfm;
    SetPaddingWords(xfm);
    GetCachedTunnelObject()->ProjectSectionToCameraSpace(
        nLane, &xfm, flStartFrame, flStartBlend, kTangentScale);
    points[kFirstSegmentPoint] = xfm.mTranslation;
    GetCachedTunnelObject()->ProjectSectionToCameraSpace(
        nLane, &xfm, flEndFrame, flEndBlend, kTangentScale);
    points[kLastSegmentPoint] = xfm.mTranslation;
    SubdivideSegment(nLane,
                     points,
                     kFirstSegmentPoint,
                     kLastSegmentPoint,
                     flStartFrame,
                     flStartBlend,
                     flEndFrame,
                     flEndBlend);

    DurGemRowString *pRow = GetRowString(nLane, nRow);
    const Vector3 &middle = points[kMiddleSegmentPoint];
    if (middle.x == 0.0f && middle.y == 0.0f && middle.z == 0.0f) {
        pRow->AddLine(color, flStartFrame, flStartBlend, flEndFrame, flEndBlend);
        return;
    }

    Rnd::Mat *pMat = pRow->mString->GetMat();
    const float flWidth = pRow->mString->GetWidth();
    auto it = mCurves.insert(FindFirstCurve(mCurves, nRow), DurGemCurve());
    it->Init(mView, nLane, nRow, points, color, pMat, flWidth);
}

// 0x00433530
void DurGemTrails::SubdivideSegment(int nLane,
                                    Vector3 *pPoints,
                                    int nFirst,
                                    int nLast,
                                    float flStartFrame,
                                    float flStartBlend,
                                    float flEndFrame,
                                    float flEndBlend) {
    const int nMid = static_cast<int>((nFirst + nLast) * 0.5f);
    if (nMid == nFirst || nMid == nLast) {
        return;
    }
    // The binary also compares the x of the midpoint and of both ends with 0 and discards the
    // results.
    float flMidFrame;
    float flMidBlend;
    Vector3 midPoint;
    if (NeedsSubdivision(nLane,
                         pPoints[nFirst],
                         pPoints[nLast],
                         &flMidFrame,
                         &flMidBlend,
                         &midPoint,
                         flStartFrame,
                         flStartBlend,
                         flEndFrame,
                         flEndBlend)) {
        pPoints[nMid] = midPoint;
        SubdivideSegment(
            nLane, pPoints, nFirst, nMid, flStartFrame, flStartBlend, flMidFrame, flMidBlend);
        SubdivideSegment(
            nLane, pPoints, nMid, nLast, flMidFrame, flMidBlend, flEndFrame, flEndBlend);
    }
}

// 0x004378e0
bool DurGemTrails::NeedsSubdivision(int nLane,
                                    const Vector3 &first,
                                    const Vector3 &last,
                                    float *pMidFrame,
                                    float *pMidBlend,
                                    Vector3 *pMidPoint,
                                    float flStartFrame,
                                    float flStartBlend,
                                    float flEndFrame,
                                    float flEndBlend) {
    const Vector3 start = first;
    const Vector3 end = last;
    *pMidFrame = (flStartFrame + flEndFrame) * 0.5f;
    *pMidBlend = (flStartBlend + flEndBlend) * 0.5f;

    Transform xfm;
    SetPaddingWords(xfm);
    GetCachedTunnelObject()->ProjectSectionToCameraSpace(
        nLane, &xfm, *pMidFrame, *pMidBlend, kTangentScale);
    *pMidPoint = xfm.mTranslation;

    Vector3 offset;
    offset.w = 1.0f;
    AddVec3(&start.x, &end.x, &offset.x);
    Vec3Scale(&offset.x, 0.5f, &offset.x);
    Vec3Sub(&offset.x, &pMidPoint->x, &offset.x);
    return kMinOffsetSquared < offset.x * offset.x + offset.y * offset.y + offset.z * offset.z;
}

// 0x00433c40
void DurGemTrails::EndTrail(int nLane, int nRow) {
    DurGemRowString *pRow = GetRowString(nLane, nRow);
    if (pRow != nullptr) {
        pRow->Clear();
    }
    auto it = FindFirstCurve(mCurves, nRow);
    while (it != mCurves.end() && it->mRow == nRow) {
        if (it->mLane == nLane) {
            it = mCurves.erase(it);
        } else {
            ++it;
        }
    }
}

// 0x00433db0
void DurGemTrails::StartStrip(
    int nLane, const Color &color, int nId, float flFrame, float flBlend) {
    const DurGemRowString *pTemplate = mRows[nLane];
    Rnd::Mat *pMat = pTemplate->mString->GetMat();
    const float flWidth = pTemplate->mString->GetWidth();
    for (DurGemStrip *pStrip : mStrips) {
        if (pStrip->Start(nLane, color, nId, flFrame, flBlend, flWidth, pMat)) {
            break;
        }
    }
}

// 0x00437a60
void DurGemTrails::StopStrip(int nId, float flFrame) {
    for (DurGemStrip *pStrip : mStrips) {
        if (pStrip->Stop(nId, flFrame)) {
            break;
        }
    }
}

// 0x00433f80
void DurGemTrails::Update(float flFrame) {
    const int nFirstRow = static_cast<int>((flFrame - kLookBehindFrames) / kSliceFrames);
    const int nEndRow = nFirstRow + mRowCount;
    int nPointCount = 0;

    auto it = mCurves.begin();
    while (it != mCurves.end() && it->mRow < nFirstRow) {
        it = mCurves.erase(it);
    }

    Transform path;
    SetPaddingWords(path);
    GetCachedTunnelObject()->GetPathXfm(&path, flFrame);
    const Vector3 &normal = path.mBasisY;
    const Vector3 &origin = path.mTranslation;
    const Plane plane{normal.x,
                      normal.y,
                      normal.z,
                      -(normal.x * origin.x + normal.y * origin.y + normal.z * origin.z)};
    Vector3 hit;
    hit.w = 1.0f;

    for (int nRow = nFirstRow; nRow < nEndRow; ++nRow) {
        const bool bNearRow = nRow == nFirstRow || nRow == nFirstRow + 1;
        const int nBase = WrapIndex(nRow, mRowCount) * mLaneCount;
        for (int nLane = 0; nLane < mLaneCount; ++nLane) {
            DurGemRowString *pRow = mRows[nBase + nLane];
            if (pRow == nullptr) {
                continue;
            }
            if (nPointCount < mMaxPoints) {
                nPointCount += pRow->Show(nRow);
                if (bNearRow && pRow->FindCrossing(plane, &hit)) {
                    mTunnel->PlaceStringFlare(hit);
                }
            } else {
                pRow->mString->SetShowing(0);
            }
        }
        for (; it != mCurves.end() && it->mRow <= nRow; ++it) {
            if (nPointCount < mMaxPoints) {
                nPointCount += it->Show();
                if (bNearRow && it->FindCrossing(plane, &hit)) {
                    mTunnel->PlaceStringFlare(hit);
                }
            } else {
                it->Hide();
            }
        }
    }

    for (DurGemStrip *pStrip : mStrips) {
        pStrip->Update(flFrame);
        if (pStrip->GetHeadPos(flFrame, &hit)) {
            mTunnel->PlaceStringFlare(hit);
        }
    }
}

// 0x00436e38
HxStr DurGemTrails::NewStringName() {
    return HxStr(FormatString(kStringNameFormat, ++g_nDurGemStringCount));
}

// 0x00437af8
DurGemRowString *DurGemTrails::GetRowString(int nLane, int nRow) {
    return mRows[WrapIndex(nRow, mRowCount) * mLaneCount + nLane];
}

// 0x00436e88
std::list<DurGemCurve>::iterator DurGemTrails::FindFirstCurve(std::list<DurGemCurve> &curves,
                                                              int nRow) {
    auto it = curves.begin();
    while (it != curves.end() && it->mRow < nRow) {
        ++it;
    }
    return it;
}
