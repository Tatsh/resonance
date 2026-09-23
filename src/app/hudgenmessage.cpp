#include "app/hudgenmessage.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"

HudGenMessage::HudGenMessage(const HxStr &name) : mText(nullptr) {
    mText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(name));
    mText->SetShowing(0);
}

void HudGenMessage::Show(const HxStr &text) {
    mText->SetText(text);
    mText->SetShowing(1);
}

void HudGenMessage::Hide() {
    mText->SetShowing(0);
}
