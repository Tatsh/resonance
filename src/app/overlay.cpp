#include "app/overlay.h"

#include "app/application.h"
#include "app/hudbadge.h"
#include "app/hudtrack.h"
#include "game/gamemanagerimpl.h"
#include "os/formatstring.h"
#include "script/scripthost.h"

namespace {

// Script template a GameOverMsg runs when mUnknown44 is set.
constexpr int kGameOverScriptTemplate = 1001;

// Script template a JamEffectMsg runs in kPlayModeJam when mUnknown44 is set.
constexpr int kJamEffectScriptTemplate = 1017;

} // namespace

Overlay *g_pOverlay;
HxStr g_hudLayoutName;

void Overlay::OnBarChanged(int nTrack, int nBar, long long llValue) {
    if (nBar != mUnknown4c) {
        return;
    }

    for (std::vector<HudTrack *>::iterator it = mUnknown08.begin(); it != mUnknown08.end(); ++it) {
        if ((*it)->mUnknowne4 == nTrack) {
            (*it)->mEffects.SetMask(llValue);
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

HudBadge *Overlay::FindBadge(Player *pPlayer) {
    for (std::vector<HudBadge *>::iterator it = mUnknown14.begin(); it != mUnknown14.end(); ++it) {
        if ((*it)->mPlayer == pPlayer) {
            return *it;
        }
    }
    return nullptr;
}

void Overlay::SetLayoutName(int nLayout) {
    g_hudLayoutName = FormatString("HUD%d", nLayout);
}
