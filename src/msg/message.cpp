#include "msg/message.h"

#include <iostream>

Message::~Message() {
}

void Message::PrintExtra(std::ostream &) const {
}

void Message::saveGuts(OBStream &) const {
}

void Message::restoreGuts(IBStream &) {
}
