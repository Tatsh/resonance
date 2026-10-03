#include "gs/ghostnoteseffector.h"

#include "msg/message.h"

// NTSC-U/C: 0x001a1cf8, PAL: 0x001a7a60
GhostNotesEffector::~GhostNotesEffector() {
}

// NTSC-U/C: 0x001a1e30, PAL: 0x001a7b98
int GhostNotesEffector::Type() {
    return kEffectorTypeGhostNotes;
}

// NTSC-U/C: 0x001a1e38, PAL: 0x001a7ba0
void GhostNotesEffector::SetEnabled([[maybe_unused]] int bEnabled) {
}
