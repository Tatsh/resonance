#include "msg/enablefreestylemsg.h"

// 0x003d7730
Message *EnableFreestyleMsg::New() {
    return new EnableFreestyleMsg;
}

// 0x001ca8c8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *EnableFreestyleMsg::Clone() {
    return new EnableFreestyleMsg(*this);
}

// 0x001ca930
int EnableFreestyleMsg::Type() {
    return g_nEnableFreestyleMsgType;
}

// 0x001ca940
const char *EnableFreestyleMsg::Name() {
    return "EnableFreestyleMsg";
}
