#include "os/spewregistrar.h"

#include "os/spew.h"

SpewRegistrar::SpewRegistrar(std::ostream **ppStream, const char *pszFile) {
    Spew::shared().Register(ppStream, pszFile);
}
