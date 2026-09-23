#include "app/hudbadge.h"

// 0x0042ac58
HudBadge::HudBadge(Player *pPlayer, int nIndex)
    : mScore(pPlayer, nIndex), mFreq(pPlayer, nIndex), mPlayer(pPlayer) {
}

void HudBadge::SetFrame(float flFrame, float flTime) {
    mFreq.SetFrame(flFrame);
    mScore.Update(flTime);
}
