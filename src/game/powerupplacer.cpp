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
void PowerupPlacer::OnUnknownSlot4() {
}

// 0x001cd998
void PowerupPlacer::OnUnknownSlot5() {
}

// 0x001cd9a0
void PowerupPlacer::OnUnknownSlot6(int) {
}

// 0x001cd9a8
void PowerupPlacer::OnUnknownSlot7() {
}

// 0x001ce1b0
void PowerupPlacer::OnUnknownSlot8() {
}
