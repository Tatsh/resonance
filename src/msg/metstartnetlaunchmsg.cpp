#include "msg/metstartnetlaunchmsg.h"

#include <iostream>

// NTSC-U/C: 0x003d7cd8, PAL: 0x0040fbf0
Message *MetStartNetLaunchMsg::New() {
    return new MetStartNetLaunchMsg;
}

// NTSC-U/C: 0x003e2960, PAL: 0x0041ae00
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *MetStartNetLaunchMsg::Clone() {
    return new MetStartNetLaunchMsg(*this);
}

// NTSC-U/C: 0x003e2998, PAL: 0x0041ae38
int MetStartNetLaunchMsg::Type() {
    return g_nMetStartNetLaunchMsgType;
}

// NTSC-U/C: 0x003e29a8, PAL: 0x0041ae48
const char *MetStartNetLaunchMsg::GetName() const {
    return "MetStartNetLaunchMsg";
}

// NTSC-U/C: 0x003e4460, PAL: 0x0041c690
void MetStartNetLaunchMsg::PrintExtra(std::ostream &stream) const {
    stream << "MetStartNetLaunchMsg";
}
