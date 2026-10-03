#include "msg/invalidateseekermsg.h"

// NTSC-U/C: 0x003d7280, PAL: 0x0040f180
Message *InvalidateSeekerMsg::New() {
    return new InvalidateSeekerMsg;
}

// NTSC-U/C: 0x001160b0, PAL: 0x00116558
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *InvalidateSeekerMsg::Clone() {
    return new InvalidateSeekerMsg(*this);
}

// NTSC-U/C: 0x00116100, PAL: 0x001165a8
int InvalidateSeekerMsg::Type() {
    return g_nInvalidateSeekerMsgType;
}

// NTSC-U/C: 0x00116110, PAL: 0x001165b8
const char *InvalidateSeekerMsg::Name() {
    return "InvalidateSeekerMsg";
}
