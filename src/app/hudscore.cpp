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
    if (mChangeTime == kScoreDrawn) {
        return;
    }

    if (mChangeTime == kScoreChangeUntimed) {
        mChangeTime = flTime;
    }

    if (flTime - mChangeTime > kRedrawDelay) {
        mText->SetText(HxStr(FormatString("%d", mScore)));
        mChangeTime = kScoreDrawn;
    }
}
