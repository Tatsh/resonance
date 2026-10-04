#include "os/spewregister.h"

#include "os/spewtable.h"

SpewRegister::SpewRegister(std::ostream **ppStream, const char *pszFile) {
    SpewTable::shared().Register(ppStream, pszFile);
}
