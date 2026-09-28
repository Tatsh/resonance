#include "os/fileio.h"

#include <unistd.h>

namespace {

constexpr int kStandardInput = 0;
constexpr int kStandardOutput = 1;
constexpr int kStandardError = 2;

} // namespace

// 0x00596500
extern "C" int LibcConsoleRead(int nFile, void *pBuffer, int nLength) {
    if (nFile != kStandardInput) {
        return -1;
    }
    return static_cast<int>(read(nFile, pBuffer, static_cast<size_t>(nLength)));
}

// 0x00596480
extern "C" int LibcConsoleWrite(int nFile, const void *pBuffer, int nLength) {
    if (nFile != kStandardOutput && nFile != kStandardError) {
        return -1;
    }
    // The original writes through a DECI2 channel. The C library's descriptor writes to the same
    // TTY.
    return static_cast<int>(write(nFile, pBuffer, static_cast<size_t>(nLength)));
}

// 0x005965a0
extern "C" int LibcConsoleClose([[maybe_unused]] int nFile) {
    return -1;
}

// 0x005965b0
extern "C" int LibcConsoleLseek([[maybe_unused]] int nFile,
                                [[maybe_unused]] int nOffset,
                                [[maybe_unused]] int nOrigin) {
    return -1;
}

// 0x00596668
extern "C" int LibcConsoleIsatty([[maybe_unused]] int nFile) {
    return 1;
}
