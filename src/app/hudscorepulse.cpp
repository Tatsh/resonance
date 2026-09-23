#include "app/hudscorepulse.h"

#include <cstring>

#include "app/hudbadge.h"
#include "app/hudutil.h"
#include "app/overlay.h"
#include "game/player.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/transformable.h"

namespace {

// Row of a local transform that holds the translation.
constexpr int kXfmRowTranslation = 3;

} // namespace

// 0x0041bdb8
HudScorePulse::HudScorePulse() {
    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mMesh = dynamic_cast<Rnd::Mesh *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s score pulse.mesh", pszLayout))));
    mMesh->SetShowing(0);
}

// 0x0041c2b0
void HudScorePulse::MoveTo(HudBadge *pBadge) {
    mMesh->mMat->SetEmissive(HudColorFromName(pBadge->mPlayer->mColorName));

    // The pulse takes the translation row of the score mesh's local transform.
    const Rnd::Transformable *pScore = pBadge->mScore.mMesh;
    std::memcpy(mMesh->mLocalXfm[kXfmRowTranslation],
                pScore->mLocalXfm[kXfmRowTranslation],
                sizeof(mMesh->mLocalXfm[kXfmRowTranslation]));
    mMesh->mDirty = 1;
    mMesh->SetShowing(1);
}
