#include "game/powerupplacer.h"

// NTSC-U/C: 0x001cd958, PAL: 0x001d3810
// The body adds nothing to the MsgSource construction the compiler expands ahead of
// the table store.
PowerupPlacer::PowerupPlacer() {
}

// NTSC-U/C: 0x001ce0b0, PAL: 0x001d3f68
// Everything left in the body is the expansion of the MsgSource destructor.
PowerupPlacer::~PowerupPlacer() {
}

// NTSC-U/C: 0x001cd990, PAL: 0x001d3848
void PowerupPlacer::Activate() {
}

// NTSC-U/C: 0x001cd998, PAL: 0x001d3850
void PowerupPlacer::Deactivate() {
}

// NTSC-U/C: 0x001cd9a0, PAL: 0x001d3858
void PowerupPlacer::MoveCursor(int) {
}

// NTSC-U/C: 0x001cd9a8, PAL: 0x001d3860
void PowerupPlacer::AnnounceCursor() {
}

// NTSC-U/C: 0x001ce1b0, PAL: 0x001d4068
void PowerupPlacer::DeployPowerup() {
}
