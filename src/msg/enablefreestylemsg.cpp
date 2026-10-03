#include "msg/enablefreestylemsg.h"

// NTSC-U/C: 0x003d7730, PAL: 0x0040f630
Message *EnableFreestyleMsg::New() {
    return new EnableFreestyleMsg;
}

// NTSC-U/C: 0x001ca8c8, PAL: 0x001d0780
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *EnableFreestyleMsg::Clone() {
    return new EnableFreestyleMsg(*this);
}

// NTSC-U/C: 0x001ca930, PAL: 0x001d07e8
int EnableFreestyleMsg::Type() {
    return g_nEnableFreestyleMsgType;
}

// NTSC-U/C: 0x001ca940, PAL: 0x001d07f8
const char *EnableFreestyleMsg::GetName() const {
    return "EnableFreestyleMsg";
}
