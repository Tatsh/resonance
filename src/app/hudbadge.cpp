#include "app/hudbadge.h"

HudBadge::HudBadge(Player *pPlayer, int nIndex)
    : mScore(pPlayer, nIndex), mFreq(pPlayer, nIndex), mPlayer(pPlayer) {
}

void HudBadge::SetFrame(float flFrame, float flTime) {
    mFreq.SetFrame(flFrame);
    mScore.Update(flTime);
}
