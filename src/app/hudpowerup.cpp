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

// NTSC-U/C: 0x004167d0, PAL: 0x004503d0
HudPowerup::HudPowerup(int nIndex) : mContainer(nullptr) {
    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    Rnd::Mesh *pMesh = dynamic_cast<Rnd::Mesh *>(
        Rnd::TheManager.Find(HxStr(FormatString("%s pup%d.mesh", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mContainer = dynamic_cast<Rnd::View *>(
        Rnd::TheManager.Find(HxStr(FormatString("%s pup%d.view", pszLayout, nIndex))));
    mAutocatcherView = dynamic_cast<Rnd::View *>(
        Rnd::TheManager.Find(HxStr(FormatString("HUD pup auto%d.view", nIndex))));
    mNeutralizerView = dynamic_cast<Rnd::View *>(
        Rnd::TheManager.Find(HxStr(FormatString("HUD pup neut%d.view", nIndex))));
    mBumperView = dynamic_cast<Rnd::View *>(
        Rnd::TheManager.Find(HxStr(FormatString("HUD pup bump%d.view", nIndex))));
    mCripplerView = dynamic_cast<Rnd::View *>(
        Rnd::TheManager.Find(HxStr(FormatString("HUD pup crip%d.view", nIndex))));
    mFreestylerView = dynamic_cast<Rnd::View *>(
        Rnd::TheManager.Find(HxStr(FormatString("HUD pup free%d.view", nIndex))));
    mMultiplierView = dynamic_cast<Rnd::View *>(
        Rnd::TheManager.Find(HxStr(FormatString("HUD pup mult%d.view", nIndex))));

    pMesh->SetShowing(Application::shared()->GetPlayMode() == kPlayModeGame);
    Show(kHudItemNone);
}

// NTSC-U/C: 0x004299a0, PAL: 0x00464fe0
void HudPowerup::Show(int nKind) {
    Rnd::View *pView = nullptr;

    mContainer->RemoveAllDraws();
    mContainer->RemoveAllAnims();
    mContainer->RemoveAllTranses();

    switch (nKind) {
    case kHudItemNone:
        pView = nullptr;
        break;
    case kHudItemNeutralizer:
        pView = mNeutralizerView;
        break;
    case kHudItemCrippler:
        pView = mCripplerView;
        break;
    case kHudItemFreestyler:
        pView = mFreestylerView;
        break;
    case kHudItemAutocatcher:
        pView = mAutocatcherView;
        break;
    case kHudItemBumper:
        pView = mBumperView;
        break;
    case kHudItemMultiplier:
        pView = mMultiplierView;
        break;
    default:
        break;
    }

    if (pView != nullptr) {
        pView->SetShowing(1);
        mContainer->AddDraw(pView, nullptr);
        mContainer->AddTrans(pView);
        mContainer->AddAnim(pView);
    }
}
