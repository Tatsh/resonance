#include "app/durgemcurve.h"

#include "app/durgemtrails.h"
#include "math/color.h"
#include "math/plane.h"
#include "math/vector3.h"
#include "rnd/string.h"
#include "rnd/view.h"

namespace {

// out.xyz = a.xyz * flA + b.xyz * flB, with out.w taken from a. A VU0 multiply and accumulate in
// the image.
inline void BlendVector(const Vector3 &a, float flA, const Vector3 &b, float flB, Vector3 &out) {
    out.x = a.x * flA + b.x * flB;
    out.y = a.y * flA + b.y * flB;
    out.z = a.z * flA + b.z * flB;
    out.w = a.w;
}

} // namespace

DurGemCurve::~DurGemCurve() {
    delete mString;
}

void DurGemCurve::Init(Rnd::View *pView,
                       int nLane,
                       int nRow,
                       const Vector3 *pPoints,
                       const Color &color,
                       Rnd::Mat *pMat,
                       float flWidth) {
    mLane = nLane;
    mRow = nRow;
    mString = Rnd::String::New(DurGemTrails::NewStringName());
    mString->SetLinePairs(0);
    mString->SetWidth(flWidth);
    mString->SetMat(pMat);
    mString->SetShowing(0);
    pView->AddDraw(mString);

    int nCount = 0;
    for (int i = 0; i < DurGemTrails::kSegmentPointCount; ++i) {
        const Vector3 &point = pPoints[i];
        if (point.x == 0.0f && point.y == 0.0f && point.z == 0.0f) {
            continue;
        }
        mString->SetNumPoints(nCount + 1);
        mString->SetPointPos(nCount, point);
        mString->SetPointColor(nCount, color);
        ++nCount;
    }
}

int DurGemCurve::Show() {
    mString->SetShowing(1);
    return mString->NumPoints();
}

void DurGemCurve::Hide() {
    mString->SetShowing(0);
}

bool DurGemCurve::FindCrossing(const Plane &plane, Vector3 *pOut) {
    for (int i = 0; i < mString->NumPoints() - 1; ++i) {
        const Vector3 segment[] = {*mString->GetPointPos(i), *mString->GetPointPos(i + 1)};
        float flT;
        if (IntersectSegmentWithPlane(segment, plane, &flT)) {
            BlendVector(segment[1], flT, segment[0], 1.0f - flT, *pOut);
            return true;
        }
    }
    return false;
}
