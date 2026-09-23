#include "game/powerupcollectioni.h"

// 0x001cc7d0
// The body adds nothing to the MsgSource construction the compiler expands ahead of
// the table store.
PowerupCollectionI::PowerupCollectionI() {
}

// 0x001cca90
// Everything left in the body is the expansion of the MsgSource destructor.
PowerupCollectionI::~PowerupCollectionI() {
}

// 0x001ccb40
void PowerupCollectionI::AddPowerup(int) {
}

// 0x001ccb48
void PowerupCollectionI::SelectRelative(int) {
}

// 0x001ccb50
void PowerupCollectionI::Select(int) {
}

// 0x001ccb58
void PowerupCollectionI::Deploy(int, int) {
}

// 0x001ccb60
int PowerupCollectionI::HasSelection() {
    // The image leaves the return register untouched here, so the value is indeterminate.
    return 0;
}

// 0x001ccb68
void PowerupCollectionI::AnnounceState() {
}
