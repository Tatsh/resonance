#include "app/hudgenmessage.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"

// NTSC-U/C: 0x0042a6d8, PAL: 0x00465a28
HudGenMessage::HudGenMessage(const HxStr &name) : mText(nullptr) {
    mText = dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(name));
    mText->SetShowing(0);
}

// NTSC-U/C: 0x0042a768, PAL: 0x00465ab8
void HudGenMessage::Show(const HxStr &text) {
    mText->SetText(text);
    mText->SetShowing(1);
}

// NTSC-U/C: 0x0042a7c0, PAL: 0x00465b10
void HudGenMessage::Hide() {
    mText->SetShowing(0);
}
