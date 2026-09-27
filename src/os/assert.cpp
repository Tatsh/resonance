#include "os/assert.h"

#include <stdio.h>
#include <stdlib.h>

namespace {

// The report writes this format through the error stream.
constexpr char kAssertFormat[] = "assertion \"%s\" failed: file \"%s\", line %d\n";

} // namespace

// 0x0055c548
// Report the failed expression through the error stream and abort.
extern "C" void HxAssertFailed(const char *pszFile, int nLine, const char *pszExpression) {
    fprintf(stderr, kAssertFormat, pszExpression, pszFile, nLine);
    abort();
}
