#include "app/hudscore.h"

#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/text.h"

namespace {

// Pending time that marks the readout as up to date.
constexpr float kScoreDrawn = 1.0e9f;

// Pending time that marks a change whose arrival is not yet recorded.
constexpr float kScoreChangeUntimed = -1.0f;

// Time a change settles for before the readout redraws.
constexpr float kRedrawDelay = 600.0f;

} // namespace

void HudScore::Update(float flTime) {
    if (mUnknown08 == kScoreDrawn) {
        return;
    }

    if (mUnknown08 == kScoreChangeUntimed) {
        mUnknown08 = flTime;
    }

    if (flTime - mUnknown08 > kRedrawDelay) {
        mUnknown04->SetText(HxStr(FormatString("%d", mUnknown0c)));
        mUnknown08 = kScoreDrawn;
    }
}
