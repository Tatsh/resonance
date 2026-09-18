#include "msg/rawcontrollermsg.h"

// 0x003da200. The field copies are the compiler expanding the implicit copy
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
