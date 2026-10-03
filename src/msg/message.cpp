#include "msg/message.h"

#include <iostream>

// NTSC-U/C: 0x001051c0, PAL: 0x001051c0
Message::~Message() {
}

// NTSC-U/C: 0x001051f0, PAL: 0x001051f0
void Message::Print(std::ostream &) {
}

// NTSC-U/C: 0x001051f8, PAL: 0x001051f8
void Message::Save(OBStream &) {
}

// NTSC-U/C: 0x00105200, PAL: 0x00105200
void Message::Load(IBStream &) {
}
