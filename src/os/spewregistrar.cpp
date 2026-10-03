#include "os/spewregistrar.h"

#include "os/spew.h"

// NTSC-U/C: 0x004b44c8, PAL: 0x004f27d8
SpewRegistrar::SpewRegistrar(std::ostream **ppStream, const char *pszFile) {
    Spew::shared().Register(ppStream, pszFile);
}
