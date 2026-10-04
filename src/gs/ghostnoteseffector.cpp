#include "gs/ghostnoteseffector.h"

#include "msg/message.h"

GhostNotesEffector::~GhostNotesEffector() {
}

int GhostNotesEffector::Type() {
    return kEffectorTypeGhostNotes;
}

void GhostNotesEffector::SetEnabled([[maybe_unused]] int bEnabled) {
}
