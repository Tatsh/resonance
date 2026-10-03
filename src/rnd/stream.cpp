#include "rnd/stream.h"

#include "os/hxstr.h"

namespace Rnd {

// Instalment buffer at 0x008952e0, shared by every stream.
constexpr int kNameBufferSize = 0x100;

static char g_szNameBuffer[kNameBufferSize];

// NTSC-U/C: 0x0050fb60, PAL: 0x0054f148
Stream &Stream::Read(void *pDest, int nSize) {
    return ReadBytes(pDest, nSize);
}

// NTSC-U/C: 0x0050fb88, PAL: 0x0054f170
Stream &Stream::Write(const void *pSrc, int nSize) {
    return WriteBytes(pSrc, nSize);
}

// The image emits no out-of-line copy for the base destructor.
// The base stores no member, so the body performs no work.
Stream::~Stream() {
}

// NTSC-U/C: 0x0050f140, PAL: 0x0054e6e8
Stream &Stream::ReadString(HxStr &name) {
    name.Clear();

    char *pCursor = g_szNameBuffer;
    for (;;) {
        ReadBytes(pCursor, 1);
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
