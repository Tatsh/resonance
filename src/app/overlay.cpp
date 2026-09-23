#include "app/overlay.h"

#include "app/hudbadge.h"
#include "os/formatstring.h"
#include "script/scripthost.h"

namespace {

// Script template a GameOverMsg runs when mUnknown44 is set.
constexpr int kGameOverScriptTemplate = 1001;

} // namespace

Overlay *g_pOverlay;
HxStr g_hudLayoutName;

void Overlay::OnGameOver() {
    if (mUnknown44 != 0) {
        CallScriptTemplate(kGameOverScriptTemplate);
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
