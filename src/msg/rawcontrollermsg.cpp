#include "msg/rawcontrollermsg.h"

#include <iostream>

// 0x003d6868
Message *RawControllerMsg::New() {
    return new RawControllerMsg;
}

// 0x003da200
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *RawControllerMsg::Clone() {
    return new RawControllerMsg(*this);
}

// 0x003da268
int RawControllerMsg::Type() {
    return g_nRawControllerMsgType;
}

// 0x003da278
const char *RawControllerMsg::Name() {
    return "RawControllerMsg";
}

// 0x003e2fb0
void RawControllerMsg::Print(std::ostream &stream) {
    mReading.Print(stream);
}
