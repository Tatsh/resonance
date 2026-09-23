#include "app/hudgenmessage.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"

// 0x0042a6d8
HudGenMessage::HudGenMessage(const HxStr &name) : mText(nullptr) {
    mText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(name));
    mText->SetShowing(0);
}

// 0x0042a768
void HudGenMessage::Show(const HxStr &text) {
    mText->SetText(text);
    mText->SetShowing(1);
}

// 0x0042a7c0
void HudGenMessage::Hide() {
    mText->SetShowing(0);
}
