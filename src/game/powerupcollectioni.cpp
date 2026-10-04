#include "game/powerupcollectioni.h"

PowerupCollectionI::PowerupCollectionI() {
    // The body adds nothing to the MsgSource construction the compiler expands ahead of the table
    // store.
}

PowerupCollectionI::~PowerupCollectionI() {
    // Everything left in the body is the expansion of the MsgSource destructor.
}

void PowerupCollectionI::Add(PowerupType) {
}

void PowerupCollectionI::SelectRelative(int) {
}

void PowerupCollectionI::Select(int) {
}

void PowerupCollectionI::Deploy(int, int) {
}

int PowerupCollectionI::HasSelection() const {
    // The image leaves the return register untouched here, so the value is indeterminate.
    return 0;
}

void PowerupCollectionI::SendState() const {
}
