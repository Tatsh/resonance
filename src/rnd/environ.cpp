#include "rnd/environ.h"

namespace Rnd {

Environ *g_pCurrentEnviron;

int Environ::DrawSelf() {
    g_pCurrentEnviron = this;
    return 1;
}

} // namespace Rnd
