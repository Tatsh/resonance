#include "game/powerupplacer.h"

PowerupPlacer::PowerupPlacer() {
    // The body adds nothing to the MsgSource construction the compiler expands ahead of the table
    // store.
}

PowerupPlacer::~PowerupPlacer() {
    // Everything left in the body is the expansion of the MsgSource destructor.
}

void PowerupPlacer::Activate() {
}

void PowerupPlacer::Deactivate() {
}

void PowerupPlacer::MoveCursor(int) {
}

void PowerupPlacer::AnnounceCursor() {
}

void PowerupPlacer::DeployPowerup() {
}
