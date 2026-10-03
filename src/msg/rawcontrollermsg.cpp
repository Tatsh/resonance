#include "msg/rawcontrollermsg.h"

#include <iostream>

// NTSC-U/C: 0x003d6868, PAL: 0x0040e758
Message *RawControllerMsg::New() {
    return new RawControllerMsg;
}

// NTSC-U/C: 0x003da200, PAL: 0x00412638
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *RawControllerMsg::Clone() {
    return new RawControllerMsg(*this);
}

// NTSC-U/C: 0x003da268, PAL: 0x004126a0
int RawControllerMsg::Type() {
    return g_nRawControllerMsgType;
}

// NTSC-U/C: 0x003da278, PAL: 0x004126b0
const char *RawControllerMsg::GetName() const {
    return "RawControllerMsg";
}

// NTSC-U/C: 0x003e2fb0, PAL: 0x0041b450
void RawControllerMsg::PrintExtra(std::ostream &stream) const {
    mReading.Print(stream);
}
