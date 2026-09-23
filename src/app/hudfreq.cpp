#include "app/hudfreq.h"

#include "app/application.h"
#include "app/hudutil.h"
#include "app/overlay.h"
#include "game/personatexture.h"
#include "game/player.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/matanim.h"
#include "rnd/mesh.h"
#include "rnd/transanim.h"

namespace {

// The icon animation follows the song position for this many ticks and then stops.
constexpr float kIconAnimLength = 500.0f;

// How far ahead of the song position the pulse runs, and the length of one pulse loop.
constexpr float kPulseLead = 80.0f;
constexpr int kPulseLoopLength = 480;

// Loop the constructor starts the pulse in. No song position arrives at it.
constexpr int kNoPulseLoop = -100;

// Stage of the icon material that shows the persona burn texture.
constexpr int kBurnStage = 1;

} // namespace

HudFreq::HudFreq(Player *pPlayer, int nIndex) : mPulsing(0), mPulseLoop(kNoPulseLoop) {
    Rnd::Tex *pBurn = findPersonaBurnTexture(nIndex);

    mPulseAnim = dynamic_cast<Rnd::MatAnim *>(Rnd::g_manager.Find(HxStr("HUD freq pulse.mnm")));

    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mAnim = dynamic_cast<Rnd::TransAnim *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s freq%d.tnm", pszLayout, pPlayer->mId20))));

    mMat = dynamic_cast<Rnd::Mat *>(
        Rnd::g_manager.Find(HxStr(FormatString("HUD freq%d.mat", pPlayer->mId20))));
    mMat->SetEmissive(HudColorFromName(pPlayer->mColorName));
    mMat->mStages[kBurnStage].SetTex(pBurn);

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mMesh = dynamic_cast<Rnd::Mesh *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s freq%d.mesh", pszLayout, nIndex))));
    mMesh->SetMaterial(mMat);
    mMesh->SetShowing(1);
    if (Application::shared()->IsJukeboxMode()) {
        mMesh->SetShowing(0);
    }

    SetPulsing(0);
}

void HudFreq::SetPulsing(int nPulsing) {
    mPulsing = nPulsing;
}

void HudFreq::SetFrame(float flFrame) {
    if (flFrame < kIconAnimLength) {
        mAnim->SetFrame(flFrame);
    }

    flFrame += kPulseLead;
    if (mPulsing != 0) {
        mPulseLoop = static_cast<int>(flFrame / static_cast<float>(kPulseLoopLength));
    }

    // Every icon shares one pulse animation and points it at the icon material before each frame.
    mPulseAnim->SetMat(mMat);
    mPulseAnim->SetFrame(flFrame - static_cast<float>(mPulseLoop * kPulseLoopLength));
}
