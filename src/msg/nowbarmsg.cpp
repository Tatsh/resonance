#include "msg/nowbarmsg.h"

// NTSC-U/C: 0x003d7680, PAL: 0x0040f580
Message *NowBarMsg::New() {
    return new NowBarMsg;
}

// NTSC-U/C: 0x0019fa20, PAL: 0x001a5788
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *NowBarMsg::Clone() {
    return new NowBarMsg(*this);
}

// NTSC-U/C: 0x0019fa78, PAL: 0x001a57e0
int NowBarMsg::Type() {
    return g_nNowBarMsgType;
}

// NTSC-U/C: 0x0019fa88, PAL: 0x001a57f0
const char *NowBarMsg::Name() {
    return "NowBarMsg";
}
