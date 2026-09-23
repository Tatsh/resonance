#include "app/hudtcgroup.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/view.h"

HudTcGroup::HudTcGroup() {
    mView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("tc_hi_group.view")));
    SetShowing(0);
}

// 0x00429e08
void HudTcGroup::SetShowing(int nShowing) {
    mView->SetShowing(nShowing);
}
