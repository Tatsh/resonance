#include "os/spewregister.h"

#include "os/spewtable.h"

// NTSC-U/C: 0x004b44c8, PAL: 0x004f27d8
SpewRegister::SpewRegister(std::ostream **ppStream, const char *pszFile) {
    SpewTable::shared().Register(ppStream, pszFile);
}
