#include "msg/deployedpowerupmsg.h"

// 0x003d7000
Message *DeployedPowerupMsg::New() {
    return new DeployedPowerupMsg;
}

// 0x00122290
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *DeployedPowerupMsg::Clone() {
    return new DeployedPowerupMsg(*this);
}

// 0x00122300
int DeployedPowerupMsg::Type() {
    return g_nDeployedPowerupMsgType;
}

// 0x00122310
const char *DeployedPowerupMsg::Name() {
    return "DeployedPowerupMsg";
}
