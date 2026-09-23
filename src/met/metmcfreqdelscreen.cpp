#include "met/metmcfreqdelscreen.h"

#include "met/metpersonadata.h"
#include "os/hxstr.h"
#include "rnd/text.h"

namespace {

// The text a row past the end of the list shows.
static const char *const kNoText = "";

} // namespace

int MetMCFreqDelScreen::ProvideText(int nItem, int, Rnd::Text *pText, int) {
    if (static_cast<unsigned>(nItem) < mPersonas.size()) {
        pText->SetText(mPersonas[nItem]->mUnknown140.mUnknown00);
    } else {
        pText->SetText(HxStr(kNoText));
    }
    return 1;
}

int MetMCFreqDelScreen::ProvideMesh(int, int, Rnd::Mesh *, int) {
    return 1;
}

// 0x002c5d10
void MetMCFreqDelScreen::SetCardSlot(MemcardConnectState slot) {
    mUnknownbc = slot;
    mUnknowne4 = 0;
}
