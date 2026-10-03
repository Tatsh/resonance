#include "msg/deployedpowerupmsg.h"

// NTSC-U/C: 0x003d7000, PAL: 0x0040eef0
Message *DeployedPowerupMsg::New() {
    return new DeployedPowerupMsg;
}

// NTSC-U/C: 0x00122290, PAL: 0x001228a8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *DeployedPowerupMsg::Clone() {
    return new DeployedPowerupMsg(*this);
}

// NTSC-U/C: 0x00122300, PAL: 0x00122918
int DeployedPowerupMsg::Type() {
    return g_nDeployedPowerupMsgType;
}

// NTSC-U/C: 0x00122310, PAL: 0x00122928
const char *DeployedPowerupMsg::GetName() const {
    return "DeployedPowerupMsg";
}
