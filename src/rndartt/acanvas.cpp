#include "rndartt/acanvas.h"

// 0x005eb1a0
ACanvas::ACanvas(const ABitmap &bitmap) : mBitmap(bitmap) {
    mClipLeft = 0;
    mClipTop = 0;
    mClipRight = bitmap.mWidth;
    mClipBottom = bitmap.mHeight;
}

// 0x005ead68
ACanvas::~ACanvas() {
}

// 0x005eb3d0
int ACanvas::ClipCodeForPoint(int nX, int nY) const {
    int nCode = 0;
    if (nX < mClipLeft) {
        nCode |= kACanvasClipLeft;
    }
    if (nX >= mClipRight) {
        nCode |= kACanvasClipRight;
    }
    if (nY < mClipTop) {
        nCode |= kACanvasClipAbove;
    }
    if (nY >= mClipBottom) {
        nCode |= kACanvasClipBelow;
    }
    return nCode;
}
