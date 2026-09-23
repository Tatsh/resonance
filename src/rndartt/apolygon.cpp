#include "rndartt/apolygon.h"

#include "rndartt/abitmap.h"
#include "rndartt/acanvas.h"
#include "rndartt/afixed.h"
#include "rndartt/apoint.h"
#include "rndartt/apolygonedge.h"

// 0x005ebfe0
int APolygon::FindTopVertex() const {
    int nTop = 0;
    int nTopY = mPoints[mIndices[0]].mY;
    for (int i = 1; i < mVertexCount; ++i) {
        const APoint &point = mPoints[mIndices[i]];
        if (point.mY < nTopY) {
            nTop = i;
            nTopY = point.mY;
        }
    }
    return nTop;
}

// 0x005e97c8
void APolygon::SetupEdge(APolygonEdge *pEdge, short nFrom, int nDirection) const {
    pEdge->mFrom = nFrom;
    pEdge->mTo = static_cast<short>(nFrom + nDirection);
    if (pEdge->mTo < 0) {
        pEdge->mTo = static_cast<short>(mVertexCount - 1);
    } else if (pEdge->mTo == mVertexCount) {
        pEdge->mTo = 0;
    }
    pEdge->mTop = pEdge->mBottom;
    const APoint &from = mPoints[mIndices[pEdge->mFrom]];
    const APoint &to = mPoints[mIndices[pEdge->mTo]];
    pEdge->mBottom = static_cast<short>((to.mY + g_nFixedHalf) >> kACanvasFractionBits);
    pEdge->mX = from.mX;
    const int nRows = pEdge->mBottom - pEdge->mTop;
    if (nRows > 0) {
        pEdge->mStepX = (to.mX - from.mX) / nRows;
    } else {
        pEdge->mStepX = 0;
    }
}

// 0x005e9bd8
// The texture coordinates are indexed by the edge's vertex positions directly, where
// the vertex positions go through mIndices. Both match the binary.
void APolygon::SetupTexturedEdge(APolygonEdge *pEdge,
                                 short nFrom,
                                 int nDirection,
                                 const ABitmap *pTexture) const {
    SetupEdge(pEdge, nFrom, nDirection);
    const APoint &from = mTexCoords[pEdge->mFrom];
    pEdge->mTexCoord.mX = from.mX * pTexture->mWidth;
    pEdge->mTexCoord.mY = from.mY * pTexture->mHeight;
    const APoint &to = mTexCoords[pEdge->mTo];
    APoint end;
    end.mX = to.mX * pTexture->mWidth;
    end.mY = to.mY * pTexture->mHeight;
    const int nRows = pEdge->mBottom - pEdge->mTop;
    if (nRows > 0) {
        pEdge->mTexStep.mX = (end.mX - pEdge->mTexCoord.mX) / nRows;
        pEdge->mTexStep.mY = (end.mY - pEdge->mTexCoord.mY) / nRows;
    }
}
