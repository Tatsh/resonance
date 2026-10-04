#include "game/freqmakercursor.h"

#include "met/metfreqmakerassetmanager.h"
#include "rnd/mesh.h"

namespace {

// The value the scale and its steps hold before a template is placed.
constexpr float kUnsetScale = 1000.0f;
constexpr int kUnsetScaleStep = -1;
constexpr float kNoPalettePosition = -1.0f;

} // namespace

FreqMakerCursor::FreqMakerCursor()
    : mCursorX(0), mCursorZ(0), mSelected(nullptr), mSelectedDrawIndex(0), mTemplate(nullptr),
      mPlacing(1), mScaleX(kUnsetScale), mScaleZ(kUnsetScale), mScaleStepX(kUnsetScaleStep),
      mScaleStepZ(kUnsetScaleStep), mCursorMirrored(0), mPalettePosition(), mCursorMesh(nullptr) {
    mColor = g_freqMakerDefaultColor;
    mPalettePosition.y = kNoPalettePosition;
    mPalettePosition.x = kNoPalettePosition;
}

FreqMakerCursor::~FreqMakerCursor() {
    delete mCursorMesh;
    mCursorMesh = nullptr;
}

void FreqMakerCursor::deselect() {
    if (mSelected != nullptr) {
        mSelected = nullptr;
    }
}
