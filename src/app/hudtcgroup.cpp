#include "app/hudtcgroup.h"

#include "met/metstrings.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "rnd/view.h"

HudTcGroup::HudTcGroup() {
    mView = dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(HxStr("tc_hi_group.view")));
#ifdef VIDEO_STANDARD_PAL
    Rnd::Text *pPanText = dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr("tc_pan.txt")));
    pPanText->SetText(GetMetString(kMetStrIngCtrlPan));
#endif
    SetShowing(0);
}

void HudTcGroup::SetShowing(int nShowing) {
    mView->SetShowing(nShowing);
}
