#include "game/idablebase.h"

// 0x00121c68
//
// The whole routine is the vtable restore and the conditional release the compiler generates, so
// the original body is empty. IDable supplies the registry teardown.
IDableBase::~IDableBase() {
}
