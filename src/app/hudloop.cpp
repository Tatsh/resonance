#include "app/hudloop.h"

#include "app/application.h"
#include "app/overlay.h"
#include "game/gamemanagerimpl.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"

HudLoop::HudLoop(int nIndex) {
    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mWires = dynamic_cast<Rnd::Mesh *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s loopwires%d.mesh", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mIndicator = dynamic_cast<Rnd::Mesh *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s loop%d.mesh", pszLayout, nIndex))));

    mWires->SetShowing(Application::shared()->GetPlayMode() == kPlayModeJam);
    SetShowing(1);
}

void HudLoop::SetShowing(int nShowing) {
    mIndicator->SetShowing(nShowing);
}
