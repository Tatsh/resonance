#include "msg/rawcontrollermsg.h"

#include <iostream>

Message *RawControllerMsg::New() {
    return new RawControllerMsg;
}

Message *RawControllerMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new RawControllerMsg(*this);
}

int RawControllerMsg::Type() {
    return sID;
}

const char *RawControllerMsg::GetName() const {
    return "RawControllerMsg";
}

void RawControllerMsg::PrintExtra(std::ostream &stream) const {
    mReading.Print(stream);
}
