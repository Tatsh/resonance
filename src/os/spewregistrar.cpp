#include "os/spewregistrar.h"

#include "os/spew.h"

// 0x004b44c8
SpewRegistrar::SpewRegistrar(std::ostream **ppStream, const char *pszFile) {
    Spew::shared().Register(ppStream, pszFile);
}
