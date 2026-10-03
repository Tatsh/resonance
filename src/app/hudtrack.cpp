#include "app/hudtrack.h"

#include "app/application.h"
#include "app/overlay.h"
#include "game/playmap.h"
#include "os/formatstring.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x0041bec8, PAL: 0x004569f0
HudTrack::HudTrack(Player *pPlayer, int nIndex)
    : mEnergy(nIndex), mPowerup(nIndex),
      mTextMessage(HxStr(
          FormatString("%s textmsg%d",
                       g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString,
                       nIndex))),
      mLoop(nIndex), mTrackLabel(nIndex), mEffects(nIndex), mPoints(nIndex),
      mCountdown(nIndex, Application::shared()->GetPlayMap()->GetEndBar()), mBlockedCatches(0),
      mTrack(0), mDeployedPowerup(0), mPlayer(pPlayer) {
}

// NTSC-U/C: 0x0042ab08, PAL: 0x00465e58
void HudTrack::SetFrame(float flFrame, float flTime) {
    mEnergy.SetFrame(flFrame);
    mTextMessage.SetFrame(flTime);
    mPoints.SetFrame(flTime);
    mCountdown.SetFrame(flFrame);
}
