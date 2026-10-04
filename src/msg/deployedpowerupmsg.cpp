#include "msg/deployedpowerupmsg.h"

Message *DeployedPowerupMsg::New() {
    return new DeployedPowerupMsg;
}

Message *DeployedPowerupMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new DeployedPowerupMsg(*this);
}

int DeployedPowerupMsg::Type() {
    return g_nDeployedPowerupMsgType;
}

const char *DeployedPowerupMsg::GetName() const {
    return "DeployedPowerupMsg";
}
