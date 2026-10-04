#include "rnd/stream.h"

#include "os/hxstr.h"

namespace Rnd {

// Instalment buffer at 0x008952e0, shared by every stream.
constexpr int kNameBufferSize = 0x100;

static char g_szNameBuffer[kNameBufferSize];

Stream &Stream::ReadLE(void *pDest, int nSize) {
    return Read(pDest, nSize);
}

Stream &Stream::WriteLE(const void *pSrc, int nSize) {
    return Write(pSrc, nSize);
}

Stream::~Stream() {
    // The image emits no out-of-line copy for the base destructor. The base stores no member, so
    // the body performs no work.
}

Stream &Stream::ReadString(HxStr &name) {
    name.Clear();

    char *pCursor = g_szNameBuffer;
    for (;;) {
        Read(pCursor, 1);
        if (*pCursor == 0) {
            name += HxStr(g_szNameBuffer);
            return *this;
        }

        ++pCursor;
        if (pCursor - g_szNameBuffer >= kNameBufferSize) {
            name += HxStr(g_szNameBuffer);
            pCursor = g_szNameBuffer;
        }
    }
}

} // namespace Rnd
