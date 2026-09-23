#include "app/hudtracklabel.h"

#include "app/overlay.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/text.h"

HudTrackLabel::HudTrackLabel(int nIndex) {
    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mUnknown00 = dynamic_cast<Rnd::Mesh *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s track%d.mesh", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mUnknown04 = dynamic_cast<Rnd::Text *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s track%d.txt", pszLayout, nIndex))));

    mUnknown00->SetShowing(1);
}

void HudTrackLabel::SetText(const HxStr &text) {
    mUnknown04->SetText(text);
}
