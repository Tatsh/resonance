#include "app/hudeffects.h"

#include "app/application.h"
#include "app/hudutil.h"
#include "app/overlay.h"
#include "game/gamemanagerimpl.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/font.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

// Configuration code the constructor reads the list of lamp kinds from.
constexpr int kLampKindsConfigCode = 0x389;

} // namespace

// 0x00417f50
HudEffects::HudEffects(int nIndex) {
    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mWires = dynamic_cast<Rnd::Mesh *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s fxwires%d.mesh", pszLayout, nIndex))));
    mLitMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("HUD fx_on.mat")));
    mUnlitMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("HUD fx_off.mat")));
    mSelectedFont = dynamic_cast<Rnd::Font *>(Rnd::g_manager.Find(HxStr("HUD fx_on.font")));
    mPlainFont = dynamic_cast<Rnd::Font *>(Rnd::g_manager.Find(HxStr("HUD fx_off.font")));

    std::vector<int> kinds;
    QueryConfigVector(&kinds, kLampKindsConfigCode);
    int nLamp = 0;
    for (std::vector<int>::iterator it = kinds.begin(); it < kinds.end(); ++it, ++nLamp) {
        Lamp lamp;
        lamp.mKind = *it;

        pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
        lamp.mMesh = dynamic_cast<Rnd::Mesh *>(
            Rnd::g_manager.Find(HxStr(FormatString("%s fx%d%d.mesh", pszLayout, nIndex, nLamp))));

        pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
        lamp.mText = dynamic_cast<Rnd::Text *>(
            Rnd::g_manager.Find(HxStr(FormatString("%s fx%d%d.txt", pszLayout, nIndex, nLamp))));

        lamp.mText->SetText(HudPowerupName(lamp.mKind));
        mLamps.push_back(lamp);
        SetLit(lamp.mKind, 0);
    }

    mWires->SetShowing(Application::shared()->GetPlayMode() == kPlayModeJam);
}

// 0x00418760
void HudEffects::SetMask(BarStatusMsg::Effects effects) {
    for (std::vector<Lamp>::iterator it = mLamps.begin(); it != mLamps.end(); ++it) {
        if (it->mKind == kHudItemGuides) {
            continue;
        }
        it->mMesh->SetMaterial(effects[it->mKind] ? mLitMat : mUnlitMat);
    }
}

// 0x00429e98
void HudEffects::Select(int nKind) {
    for (std::vector<Lamp>::iterator it = mLamps.begin(); it != mLamps.end(); ++it) {
        it->mText->SetFont(it->mKind == nKind ? mSelectedFont : mPlainFont);
    }
}

// 0x00429f28
void HudEffects::SetLit(int nKind, int nLit) {
    for (std::vector<Lamp>::iterator it = mLamps.begin(); it != mLamps.end(); ++it) {
        if (it->mKind == nKind) {
            it->mMesh->SetMaterial(nLit != 0 ? mLitMat : mUnlitMat);
        }
    }
}
