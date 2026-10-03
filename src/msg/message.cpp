#include "msg/message.h"

#include <iostream>

// NTSC-U/C: 0x001051c0, PAL: 0x001051c0
Message::~Message() {
}

// NTSC-U/C: 0x001051f0, PAL: 0x001051f0
void Message::PrintExtra(std::ostream &) const {
}

// NTSC-U/C: 0x001051f8, PAL: 0x001051f8
void Message::saveGuts(OBStream &) const {
}

// NTSC-U/C: 0x00105200, PAL: 0x00105200
void Message::restoreGuts(IBStream &) {
}
