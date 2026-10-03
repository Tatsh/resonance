#include "msg/advancesectiontogglemsg.h"

// NTSC-U/C: 0x003d71d0, PAL: 0x0040f0d0
Message *AdvanceSectionToggleMsg::New() {
    return new AdvanceSectionToggleMsg;
}

// NTSC-U/C: 0x00115f90, PAL: 0x00116438
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AdvanceSectionToggleMsg::Clone() {
    return new AdvanceSectionToggleMsg(*this);
}

// NTSC-U/C: 0x00115fe0, PAL: 0x00116488
int AdvanceSectionToggleMsg::Type() {
    return g_nAdvanceSectionToggleMsgType;
}

// NTSC-U/C: 0x00115ff0, PAL: 0x00116498
const char *AdvanceSectionToggleMsg::Name() {
    return "AdvanceSectionToggleMsg";
}
