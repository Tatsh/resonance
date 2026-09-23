#include "app/hudpowerup.h"

#include "app/application.h"
#include "app/hudutil.h"
#include "app/overlay.h"
#include "game/gamemanagerimpl.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/view.h"

HudPowerup::HudPowerup(int nIndex) : mUnknown18(nullptr) {
    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    Rnd::Mesh *pMesh = dynamic_cast<Rnd::Mesh *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s pup%d.mesh", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mUnknown18 = dynamic_cast<Rnd::View *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s pup%d.view", pszLayout, nIndex))));
    mUnknown00 = dynamic_cast<Rnd::View *>(
        Rnd::g_manager.Find(HxStr(FormatString("HUD pup auto%d.view", nIndex))));
    mUnknown04 = dynamic_cast<Rnd::View *>(
        Rnd::g_manager.Find(HxStr(FormatString("HUD pup neut%d.view", nIndex))));
    mUnknown08 = dynamic_cast<Rnd::View *>(
        Rnd::g_manager.Find(HxStr(FormatString("HUD pup bump%d.view", nIndex))));
    mUnknown0c = dynamic_cast<Rnd::View *>(
        Rnd::g_manager.Find(HxStr(FormatString("HUD pup crip%d.view", nIndex))));
    mUnknown10 = dynamic_cast<Rnd::View *>(
        Rnd::g_manager.Find(HxStr(FormatString("HUD pup free%d.view", nIndex))));
    mUnknown14 = dynamic_cast<Rnd::View *>(
        Rnd::g_manager.Find(HxStr(FormatString("HUD pup mult%d.view", nIndex))));

    pMesh->SetShowing(Application::shared()->GetPlayMode() == kPlayModeGame);
    Show(kHudItemNone);
}

void HudPowerup::Show(int nKind) {
    Rnd::View *pView = nullptr;

    mUnknown18->ClearDraws();
    mUnknown18->ReleaseAnimsRefs();
    mUnknown18->ClearTransList();

    switch (nKind) {
    case kHudItemNone:
        pView = nullptr;
        break;
    case kHudItemNeutralizer:
        pView = mUnknown04;
        break;
    case kHudItemCrippler:
        pView = mUnknown0c;
        break;
    case kHudItemFreestyler:
        pView = mUnknown10;
        break;
    case kHudItemAutocatcher:
        pView = mUnknown00;
        break;
    case kHudItemBumper:
        pView = mUnknown08;
        break;
    case kHudItemMultiplier:
        pView = mUnknown14;
        break;
    default:
        break;
    }

    if (pView != nullptr) {
        pView->SetShowing(1);
        mUnknown18->AddDraw(pView, nullptr);
        mUnknown18->AddTrans(pView);
        mUnknown18->AddAnim(pView);
    }
}
