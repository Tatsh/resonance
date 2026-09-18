#include "rnd/stream.h"

#include "os/hxstr.h"

namespace Rnd {

// Instalment buffer at 0x008952e0, shared by every stream.
constexpr int kNameBufferSize = 0x100;

static char g_szNameBuffer[kNameBufferSize];

// 0x0050fb60
Stream &Stream::Read(void *pDest, int nSize) {
    return ReadBytes(pDest, nSize);
}

// 0x0050fb88
Stream &Stream::Write(const void *pSrc, int nSize) {
    return WriteBytes(pSrc, nSize);
}

// 0x0050f140
void Stream::ReadString(HxStr &name) {
    name.Clear();

    char *pCursor = g_szNameBuffer;
    for (;;) {
        ReadBytes(pCursor, 1);
        if (*pCursor == 0) {
            name += HxStr(g_szNameBuffer);
            return;
        }

        ++pCursor;
        if (pCursor - g_szNameBuffer >= kNameBufferSize) {
            name += HxStr(g_szNameBuffer);
            pCursor = g_szNameBuffer;
        }
    }
}

} // namespace Rnd
