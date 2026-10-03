#include "game/powerupcollectioni.h"

// NTSC-U/C: 0x001cc7d0, PAL: 0x001d2688
// The body adds nothing to the MsgSource construction the compiler expands ahead of
// the table store.
PowerupCollectionI::PowerupCollectionI() {
}

// NTSC-U/C: 0x001cca90, PAL: 0x001d2948
// Everything left in the body is the expansion of the MsgSource destructor.
PowerupCollectionI::~PowerupCollectionI() {
}

// NTSC-U/C: 0x001ccb40, PAL: 0x001d29f8
void PowerupCollectionI::AddPowerup(int) {
}

// NTSC-U/C: 0x001ccb48, PAL: 0x001d2a00
void PowerupCollectionI::SelectRelative(int) {
}

// NTSC-U/C: 0x001ccb50, PAL: 0x001d2a08
void PowerupCollectionI::Select(int) {
}

// NTSC-U/C: 0x001ccb58, PAL: 0x001d2a10
void PowerupCollectionI::Deploy(int, int) {
}

// NTSC-U/C: 0x001ccb60, PAL: 0x001d2a18
int PowerupCollectionI::HasSelection() {
    // The image leaves the return register untouched here, so the value is indeterminate.
    return 0;
}

// NTSC-U/C: 0x001ccb68, PAL: 0x001d2a20
void PowerupCollectionI::AnnounceState() {
}
