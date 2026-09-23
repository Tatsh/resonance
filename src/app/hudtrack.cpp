#include "app/hudtrack.h"

void HudTrack::SetFrame(float flFrame, float flTime) {
    mEnergy.SetFrame(flFrame);
    mTextMessage.SetFrame(flTime);
    mPoints.SetFrame(flTime);
    mCountdown.SetFrame(flFrame);
}
