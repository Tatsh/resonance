#include "msg/metstartpausemsg.h"

#include <iostream>

// NTSC-U/C: 0x003d7d10, PAL: 0x0040fc28
Message *MetStartPauseMsg::New() {
    return new MetStartPauseMsg;
}

// NTSC-U/C: 0x003e2ac0, PAL: 0x0041af60
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *MetStartPauseMsg::Clone() {
    return new MetStartPauseMsg(*this);
}

// NTSC-U/C: 0x003e2af8, PAL: 0x0041af98
int MetStartPauseMsg::Type() {
    return g_nMetStartPauseMsgType;
}

// NTSC-U/C: 0x003e2b08, PAL: 0x0041afa8
const char *MetStartPauseMsg::Name() {
    return "MetStartPauseMsg";
}

// NTSC-U/C: 0x003e4488, PAL: 0x0041c6b8
void MetStartPauseMsg::Print(std::ostream &stream) {
    stream << "MetStartPauseMsg";
}
