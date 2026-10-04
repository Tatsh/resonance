#include "msg/enablefreestylemsg.h"

Message *EnableFreestyleMsg::New() {
    return new EnableFreestyleMsg;
}

Message *EnableFreestyleMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new EnableFreestyleMsg(*this);
}

int EnableFreestyleMsg::Type() {
    return g_nEnableFreestyleMsgType;
}

const char *EnableFreestyleMsg::GetName() const {
    return "EnableFreestyleMsg";
}
