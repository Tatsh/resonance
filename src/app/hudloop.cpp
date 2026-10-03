#include "app/hudloop.h"

#include "app/application.h"
#include "app/overlay.h"
#include "game/gamemanagerimpl.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"

// NTSC-U/C: 0x00417b40, PAL: 0x00451b00
HudLoop::HudLoop(int nIndex) {
    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mWires = dynamic_cast<Rnd::Mesh *>(
        Rnd::TheManager.Find(HxStr(FormatString("%s loopwires%d.mesh", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mIndicator = dynamic_cast<Rnd::Mesh *>(
        Rnd::TheManager.Find(HxStr(FormatString("%s loop%d.mesh", pszLayout, nIndex))));

    mWires->SetShowing(Application::shared()->GetPlayMode() == kPlayModeJam);
    SetShowing(1);
}

// NTSC-U/C: 0x00429e38, PAL: 0x00465478
void HudLoop::SetShowing(int nShowing) {
    mIndicator->SetShowing(nShowing);
}
