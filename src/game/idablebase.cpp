#include "game/idablebase.h"

IDableBase::~IDableBase() {
    // The whole routine is the vtable restore and the conditional release the compiler generates.
    // IDable supplies the registry teardown.
}
