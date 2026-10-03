#include "game/freqmakercursor.h"

#include "met/metfreqmakerassetmanager.h"
#include "rnd/mesh.h"

namespace {

// The value the scale and its steps hold before a template is placed.
constexpr float kUnsetScale = 1000.0f;
constexpr int kUnsetScaleStep = -1;
constexpr float kNoPalettePosition = -1.0f;

} // namespace

// NTSC-U/C: 0x0024f300, PAL: 0x00264728
FreqMakerCursor::FreqMakerCursor()
    : mCursorX(0), mCursorZ(0), mSelected(nullptr), mSelectedDrawIndex(0), mTemplate(nullptr),
      mPlacing(1), mScaleX(kUnsetScale), mScaleZ(kUnsetScale), mScaleStepX(kUnsetScaleStep),
      mScaleStepZ(kUnsetScaleStep), mCursorMirrored(0), mPalettePosition(), mCursorMesh(nullptr) {
    mColor = g_freqMakerDefaultColor;
    mPalettePosition.y = kNoPalettePosition;
    mPalettePosition.x = kNoPalettePosition;
}

// NTSC-U/C: 0x0024f3b0, PAL: 0x002647d8
FreqMakerCursor::~FreqMakerCursor() {
    delete mCursorMesh;
    mCursorMesh = nullptr;
}

// NTSC-U/C: 0x0024f2e8, PAL: 0x00264710
void FreqMakerCursor::deselect() {
    if (mSelected != nullptr) {
        mSelected = nullptr;
    }
}
