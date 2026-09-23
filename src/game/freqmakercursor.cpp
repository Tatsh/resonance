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
    : mUnknown00(0), mUnknown04(0), mUnknown20(0), mUnknown70(0), mTemplate(nullptr), mUnknown78(1),
      mScaleX(kUnsetScale), mScaleZ(kUnsetScale), mScaleStepX(kUnsetScaleStep),
      mScaleStepZ(kUnsetScaleStep), mUnknown8c(0), mPalettePosition(), mCursorMesh(nullptr) {
    mColor = g_freqMakerDefaultColor;
    mPalettePosition.y = kNoPalettePosition;
    mPalettePosition.x = kNoPalettePosition;
}

// 0x0024f3b0
FreqMakerCursor::~FreqMakerCursor() {
    delete mCursorMesh;
    mCursorMesh = nullptr;
}
