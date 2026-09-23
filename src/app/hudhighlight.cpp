#include "app/hudhighlight.h"

#include <vector>

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/meshvert.h"

namespace {

// End time the constructor starts with, long before any real time.
constexpr float kNoEndTime = -8000.0f;

// The corner groups, in the order their vertices are listed.
enum Corner { kTopLeft = 0, kTopRight = 1, kBottomRight = 2, kBottomLeft = 3 };

// The edges of a target rectangle, in the order MoveTo() records them.
enum Edge { kEdgeLeft = 0, kEdgeTop = 1, kEdgeRight = 2, kEdgeBottom = 3 };

// The vertex of a corner group that a move steers.
constexpr int kLeadVertex = 0;

} // namespace

// 0x00417170
HudHighlight::HudHighlight() : mStarting(0), mDone(1), mEndTime(kNoEndTime) {
    mMesh = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr("HUD1 hilite_box.mesh")));
    mMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("HUD hilite_box.mat")));
    SetShowing(0);

    // The corner table is written after SetShowing(), one byte at a time.
    mCornerVerts[kTopLeft][0] = 8;
    mCornerVerts[kTopLeft][1] = 9;
    mCornerVerts[kTopLeft][2] = 13;
    mCornerVerts[kTopLeft][3] = 12;
    mCornerVerts[kTopRight][0] = 10;
    mCornerVerts[kTopRight][1] = 14;
    mCornerVerts[kTopRight][2] = 15;
    mCornerVerts[kTopRight][3] = 11;
    mCornerVerts[kBottomRight][0] = 5;
    mCornerVerts[kBottomRight][1] = 7;
    mCornerVerts[kBottomRight][2] = 6;
    mCornerVerts[kBottomRight][3] = 4;
    mCornerVerts[kBottomLeft][0] = 2;
    mCornerVerts[kBottomLeft][1] = 1;
    mCornerVerts[kBottomLeft][2] = 0;
    mCornerVerts[kBottomLeft][3] = 3;
}

// 0x00417388
void HudHighlight::MoveTo(
    float flLeft, float flTop, float flRight, float flBottom, float flDuration) {
    std::vector<Rnd::MeshVert> &verts = mMesh->mVertsOwner->mVerts;

    mTarget[kEdgeLeft] = flLeft;
    mTarget[kEdgeTop] = flTop;
    mTarget[kEdgeRight] = flRight;
    mTarget[kEdgeBottom] = flBottom;

    // The two left corners share one speed and the two right corners another.
    const Rnd::MeshVert &topLeft = verts[mCornerVerts[kTopLeft][kLeadVertex]];
    mVelocity[kTopLeft] = (flLeft - topLeft.mPoint.x) / flDuration;
    mVelocity[kBottomLeft] = mVelocity[kTopLeft];
    const Rnd::MeshVert &bottomRight = verts[mCornerVerts[kBottomRight][kLeadVertex]];
    mVelocity[kTopRight] = (flRight - bottomRight.mPoint.x) / flDuration;
    mVelocity[kBottomRight] = mVelocity[kTopRight];

    mFadeRate = static_cast<float>(1.0 / flDuration);

    const Rnd::MeshVert &lead0 = verts[mCornerVerts[kTopLeft][kLeadVertex]];
    mSlope[kTopLeft] = (lead0.mPoint.y - flTop) / (lead0.mPoint.x - flLeft);
    mIntercept[kTopLeft] = flTop - mSlope[kTopLeft] * flLeft;

    const Rnd::MeshVert &lead1 = verts[mCornerVerts[kTopRight][kLeadVertex]];
    mSlope[kTopRight] = (lead1.mPoint.y - flTop) / (lead1.mPoint.x - flRight);
    mIntercept[kTopRight] = flTop - mSlope[kTopRight] * flRight;

    const Rnd::MeshVert &lead2 = verts[mCornerVerts[kBottomRight][kLeadVertex]];
    mSlope[kBottomRight] = (lead2.mPoint.y - flBottom) / (lead2.mPoint.x - flRight);
    mIntercept[kBottomRight] = flBottom - mSlope[kBottomRight] * flRight;

    const Rnd::MeshVert &lead3 = verts[mCornerVerts[kBottomLeft][kLeadVertex]];
    mSlope[kBottomLeft] = (lead3.mPoint.y - flBottom) / (lead3.mPoint.x - flLeft);
    mIntercept[kBottomLeft] = flBottom - mSlope[kBottomLeft] * flLeft;

    mAlpha = 0.0f;
    mMat->SetAlpha(mAlpha);
    mDuration = flDuration;
    mStarting = 1;
    mDone = 0;
}

// 0x00417570
void HudHighlight::JumpTo(float flLeft, float flTop, float flRight, float flBottom) {
    std::vector<Rnd::MeshVert> &verts = mMesh->mVertsOwner->mVerts;
    const Rnd::MeshVert &topLeft = verts[mCornerVerts[kTopLeft][kLeadVertex]];
    const Rnd::MeshVert &bottomRight = verts[mCornerVerts[kBottomRight][kLeadVertex]];

    float aflDeltaY[kCornerCount];
    aflDeltaY[kTopLeft] = flTop - topLeft.mPoint.y;
    aflDeltaY[kTopRight] = aflDeltaY[kTopLeft];
    aflDeltaY[kBottomRight] = flBottom - bottomRight.mPoint.y;
    aflDeltaY[kBottomLeft] = aflDeltaY[kBottomRight];

    float aflDeltaX[kCornerCount];
    aflDeltaX[kTopLeft] = flLeft - topLeft.mPoint.x;
    aflDeltaX[kBottomLeft] = aflDeltaX[kTopLeft];
    aflDeltaX[kTopRight] = flRight - bottomRight.mPoint.x;
    aflDeltaX[kBottomRight] = aflDeltaX[kTopRight];

    for (int nCorner = 0; nCorner < kCornerCount; ++nCorner) {
        for (int nVertex = 0; nVertex < kCornerVertexCount; ++nVertex) {
            Rnd::MeshVert &vert = verts[mCornerVerts[nCorner][nVertex]];
            vert.mPoint.x += aflDeltaX[nCorner];
            vert.mPoint.y += aflDeltaY[nCorner];
        }
    }
}

// 0x00417690
void HudHighlight::SetFrame(float flTime) {
    if (mStarting != 0) {
        mLastTime = flTime;
        mEndTime = flTime + mDuration;
        mStarting = 0;
        return;
    }

    float flUntil = flTime;
    if (mEndTime < flTime) {
        if (mDone != 0) {
            return;
        }
        mDone = 1;
        flUntil = mEndTime;
    }

    const float flElapsed = flUntil - mLastTime;
    std::vector<Rnd::MeshVert> &verts = mMesh->mVertsOwner->mVerts;
    for (int nCorner = 0; nCorner < kCornerCount; ++nCorner) {
        const float flStepX = mVelocity[nCorner] * flElapsed;
        const Rnd::MeshVert &lead = verts[mCornerVerts[nCorner][kLeadVertex]];
        const float flStepY =
            mSlope[nCorner] * (lead.mPoint.x + flStepX) + mIntercept[nCorner] - lead.mPoint.y;
        for (int nVertex = 0; nVertex < kCornerVertexCount; ++nVertex) {
            Rnd::MeshVert &vert = verts[mCornerVerts[nCorner][nVertex]];
            vert.mPoint.x += flStepX;
            vert.mPoint.y += flStepY;
        }
    }

    mAlpha += mFadeRate * flElapsed;
    if (mAlpha > 1.0) {
        mAlpha = 1.0f;
    }
    mMat->SetAlpha(mAlpha);
    mLastTime = flTime;
}

// 0x00429d30
void HudHighlight::SetShowing(int nShowing) {
    mMesh->SetShowing(nShowing);
}
