#include "game/powerupplacer.h"

// 0x001cd958
// The body adds nothing to the MsgSource construction the compiler expands ahead of
// the table store.
PowerupPlacer::PowerupPlacer() {
}

// 0x001ce0b0
// Everything left in the body is the expansion of the MsgSource destructor.
PowerupPlacer::~PowerupPlacer() {
}

// 0x001cd990
void PowerupPlacer::Activate() {
}

// 0x001cd998
void PowerupPlacer::Deactivate() {
}

// 0x001cd9a0
void PowerupPlacer::MoveCursor(int) {
}

// 0x001cd9a8
void PowerupPlacer::AnnounceCursor() {
}

// 0x001ce1b0
void PowerupPlacer::DeployPowerup() {
}
