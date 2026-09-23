#include "app/hudtrack.h"

#include "app/application.h"
#include "app/overlay.h"
#include "game/playmap.h"
#include "os/formatstring.h"
#include "os/hxstr.h"

HudTrack::HudTrack(Player *pPlayer, int nIndex)
    : mEnergy(nIndex), mPowerup(nIndex),
      mTextMessage(HxStr(
          FormatString("%s textmsg%d",
                       g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString,
                       nIndex))),
      mLoop(nIndex), mTrackLabel(nIndex), mEffects(nIndex), mPoints(nIndex),
      mCountdown(nIndex, Application::shared()->GetPlayMap()->Slot9()), mUnknowne0(0), mTrack(0),
      mUnknownec(0), mPlayer(pPlayer) {
}

void HudTrack::SetFrame(float flFrame, float flTime) {
    mEnergy.SetFrame(flFrame);
    mTextMessage.SetFrame(flTime);
    mPoints.SetFrame(flTime);
    mCountdown.SetFrame(flFrame);
}
