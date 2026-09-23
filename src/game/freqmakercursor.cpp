#include "game/freqmakercursor.h"

#include "met/metfreqmakerassetmanager.h"
#include "rnd/mesh.h"

namespace {

// The value the scale and its steps hold before a template is placed.
constexpr float kUnsetScale = 1000.0f;
constexpr int kUnsetScaleStep = -1;
constexpr float kNoPalettePosition = -1.0f;

} // namespace

// 0x0024f300
FreqMakerCursor::FreqMakerCursor()
    : mCursorX(0), mCursorZ(0), mSelected(nullptr), mSelectedDrawIndex(0), mTemplate(nullptr),
      mPlacing(1), mScaleX(kUnsetScale), mScaleZ(kUnsetScale), mScaleStepX(kUnsetScaleStep),
      mScaleStepZ(kUnsetScaleStep), mCursorMirrored(0), mPalettePosition(), mCursorMesh(nullptr) {
    mColor = g_freqMakerDefaultColor;
    mPalettePosition.y = kNoPalettePosition;
    mPalettePosition.x = kNoPalettePosition;
}

// 0x0024f3b0
FreqMakerCursor::~FreqMakerCursor() {
    delete mCursorMesh;
    mCursorMesh = nullptr;
}

// 0x0024f2e8
void FreqMakerCursor::deselect() {
    if (mSelected != nullptr) {
        mSelected = nullptr;
    }
}
