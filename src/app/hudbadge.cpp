#include "app/hudbadge.h"

// NTSC-U/C: 0x0042ac58, PAL: 0x00465fa8
HudBadge::HudBadge(Player *pPlayer, int nIndex)
    : mScore(pPlayer, nIndex), mFreq(pPlayer, nIndex), mPlayer(pPlayer) {
}

// NTSC-U/C: 0x0041c0f8, PAL: 0x00466008
void HudBadge::SetFrame(float flFrame, float flTime) {
    mFreq.SetFrame(flFrame);
    mScore.Update(flTime);
}
