#include "app/overlay.h"

#include "app/application.h"
#include "app/hudbadge.h"
#include "app/hudpanel.h"
#include "app/hudtrack.h"
#include "app/renderer.h"
#include "game/gamemanagerimpl.h"
#include "gfx/gfxdevice.h"
#include "os/formatstring.h"
#include "script/scripthost.h"

namespace {

// MIDI ticks in one bar.
constexpr float kTicksPerBar = 1920.0f;

// Script template a GameOverMsg runs when mUnknown44 is set.
constexpr int kGameOverScriptTemplate = 1001;

// Script template a JamEffectMsg runs in kPlayModeJam when mUnknown44 is set.
constexpr int kJamEffectScriptTemplate = 1017;

} // namespace

Overlay *g_pOverlay;
HxStr g_hudLayoutName;

Overlay::~Overlay() {
    g_pOverlay = nullptr;
    for (std::vector<HudBadge *>::iterator it = mBadges.begin(); it != mBadges.end(); ++it) {
        delete *it;
    }
    for (std::vector<HudTrack *>::iterator it = mTracks.begin(); it != mTracks.end(); ++it) {
        delete *it;
    }
    delete mPanel;
    g_gfxDevice.mFeedbackEnabled = 0;
}

void Overlay::SetFrame(float flFrame) {
    const int nBar = static_cast<int>(flFrame / kTicksPerBar);
    bool bBarChanged = false;
    if (nBar != mCurrentBar) {
        mCurrentBar = nBar;
        bBarChanged = true;
    }

    const float flTime = flFrame * mMsPerTick;
    for (std::vector<HudBadge *>::iterator it = mBadges.begin(); it != mBadges.end(); ++it) {
        (*it)->SetFrame(flFrame, flTime);
    }
    for (std::vector<HudTrack *>::iterator it = mTracks.begin(); it != mTracks.end(); ++it) {
        (*it)->SetFrame(flFrame, flTime);
        if (bBarChanged) {
            (*it)->mEffects.SetMask(mRenderer->GetCell((*it)->mTrack, mCurrentBar)->mEffects);
        }
    }
    mPanel->SetFrame(flFrame, flTime);

    if (Application::shared()->GetGameManager()->IsPlaybackActive() != 0) {
        mPanel->mMessage.Show(HxStr("DEMO\n\nPress any button to exit"));
    }
}

void Overlay::OnBarChanged(int nTrack, int nBar, BarStatusMsg::Effects effects) {
    if (nBar != mCurrentBar) {
        return;
    }

    for (std::vector<HudTrack *>::iterator it = mTracks.begin(); it != mTracks.end(); ++it) {
        if ((*it)->mTrack == nTrack) {
            (*it)->mEffects.SetMask(effects);
        }
    }
}

void Overlay::OnGameOver() {
    if (mUnknown44 != 0) {
        CallScriptTemplate(kGameOverScriptTemplate);
    }
}

void Overlay::OnJamEffect() {
    if (mUnknown44 != 0 && Application::shared()->GetPlayMode() == kPlayModeJam) {
        CallScriptTemplate(kJamEffectScriptTemplate);
    }
}

void Overlay::Draw() {
    mPanel->mWinMessage.Draw();
}

void Overlay::OnLeaderChanged(Player *pOldLeader, Player *pNewLeader) {
    if (pNewLeader == nullptr) {
        mPanel->mScorePulse.Hide();
    } else {
        mPanel->mScorePulse.MoveTo(FindBadge(pNewLeader));
    }
    if (pOldLeader != nullptr) {
        FindBadge(pOldLeader)->mFreq.SetPulsing(0);
    }
    if (pNewLeader != nullptr) {
        FindBadge(pNewLeader)->mFreq.SetPulsing(1);
    }
}

HudBadge *Overlay::FindBadge(Player *pPlayer) {
    for (std::vector<HudBadge *>::iterator it = mBadges.begin(); it != mBadges.end(); ++it) {
        if ((*it)->mPlayer == pPlayer) {
            return *it;
        }
    }
    return nullptr;
}

void Overlay::SetLayoutName(int nLayout) {
    g_hudLayoutName = FormatString("HUD%d", nLayout);
}
