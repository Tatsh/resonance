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

HudEffects::HudEffects(int nIndex) {
    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mUnknown00 = dynamic_cast<Rnd::Mesh *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s fxwires%d.mesh", pszLayout, nIndex))));
    mUnknown04 = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("HUD fx_on.mat")));
    mUnknown08 = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("HUD fx_off.mat")));
    mUnknown0c = dynamic_cast<Rnd::Font *>(Rnd::g_manager.Find(HxStr("HUD fx_on.font")));
    mUnknown10 = dynamic_cast<Rnd::Font *>(Rnd::g_manager.Find(HxStr("HUD fx_off.font")));

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
        mUnknown14.push_back(lamp);
        SetLit(lamp.mKind, 0);
    }

    mUnknown00->SetShowing(Application::shared()->GetPlayMode() == kPlayModeJam);
}

void HudEffects::SetMask(long long llMask) {
    for (std::vector<Lamp>::iterator it = mUnknown14.begin(); it != mUnknown14.end(); ++it) {
        if (it->mKind == kHudItemGuides) {
            continue;
        }
        it->mMesh->SetMaterial((llMask & (1LL << it->mKind)) != 0 ? mUnknown04 : mUnknown08);
    }
}

void HudEffects::Select(int nKind) {
    for (std::vector<Lamp>::iterator it = mUnknown14.begin(); it != mUnknown14.end(); ++it) {
        it->mText->SetFont(it->mKind == nKind ? mUnknown0c : mUnknown10);
    }
}

void HudEffects::SetLit(int nKind, int nLit) {
    for (std::vector<Lamp>::iterator it = mUnknown14.begin(); it != mUnknown14.end(); ++it) {
        if (it->mKind == nKind) {
            it->mMesh->SetMaterial(nLit != 0 ? mUnknown04 : mUnknown08);
        }
    }
}
