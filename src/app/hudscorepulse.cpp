#include "app/hudscorepulse.h"

#include "app/overlay.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"

HudScorePulse::HudScorePulse() {
    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mUnknown00 = dynamic_cast<Rnd::Mesh *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s score pulse.mesh", pszLayout))));
    mUnknown00->SetShowing(0);
}
