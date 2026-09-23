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
    mUnknown04 = dynamic_cast<Rnd::Mesh *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s loopwires%d.mesh", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mUnknown00 = dynamic_cast<Rnd::Mesh *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s loop%d.mesh", pszLayout, nIndex))));

    mUnknown04->SetShowing(Application::shared()->GetPlayMode() == kPlayModeJam);
    SetShowing(1);
}

void HudLoop::SetShowing(int nShowing) {
    mUnknown00->SetShowing(nShowing);
}
