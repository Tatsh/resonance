#include "app/hudscore.h"

#include "app/application.h"
#include "app/overlay.h"
#include "game/gamemanagerimpl.h"
#include "game/player.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/font.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/text.h"

namespace {

// Pending time that marks the readout as up to date.
constexpr float kScoreDrawn = 1.0e9f;

// Pending time that marks a change whose arrival is not yet recorded.
constexpr float kScoreChangeUntimed = -1.0f;

// Time a change settles for before the readout redraws.
constexpr float kRedrawDelay = 600.0f;

// Pending time the constructor starts with, far enough back that the first Update() redraws.
constexpr float kScoreChangeLongAgo = -100000.0f;

} // namespace

// 0x00419618
HudScore::HudScore(Player *pPlayer, int nIndex) : mText(nullptr) {
    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mMesh = dynamic_cast<Rnd::Mesh *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s score%d.mesh", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mText = dynamic_cast<Rnd::Text *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s score%d.txt", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    Rnd::Font *pFont = dynamic_cast<Rnd::Font *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s score%d.font", pszLayout, nIndex))));

    Rnd::Mat *pMat = dynamic_cast<Rnd::Mat *>(
        Rnd::g_manager.Find(HxStr("HUD score font ") + HxStr(pPlayer->mColorName) + ".mat"));
    pFont->SetMat(pMat);

    mMesh->SetShowing(Application::shared()->GetPlayMode() == kPlayModeGame);
    mScore = 0;
    mChangeTime = kScoreChangeLongAgo;
}

// 0x0042a220
void HudScore::Update(float flTime) {
    if (mChangeTime == kScoreDrawn) {
        return;
    }

    if (mChangeTime == kScoreChangeUntimed) {
        mChangeTime = flTime;
    }

    if (flTime - mChangeTime > kRedrawDelay) {
        mText->SetText(HxStr(FormatString("%d", mScore)));
        mChangeTime = kScoreDrawn;
    }
}
