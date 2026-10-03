#include "game/idablebase.h"

// NTSC-U/C: 0x00121c68, PAL: 0x00122270
//
// The whole routine is the vtable restore and the conditional release the compiler generates, so
// the original body is empty. IDable supplies the registry teardown.
IDableBase::~IDableBase() {
}
