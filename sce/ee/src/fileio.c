#include <fcntl.h>
#include <sifdev.h>
#include <string.h>
#include <unistd.h>

// Sony open flags with their newlib equivalents. FileOpen at 0x0047c9c0 maps newlib
// access 0, 1, and 2 to Sony 1, 2, and 3, newlib append 0x8 to Sony 0x100, and carries
// create 0x200 and truncate 0x400 across unchanged. strFileOpen passes Sony 1 to open
// for reading, which confirms the read-only value.
enum {
    kSceAccessMask = 3,
    kSceOpenReadOnly = 1,
    kSceOpenWriteOnly = 2,
    kSceOpenReadWrite = 3,
    kSceOpenAppend = 0x100,
    kSceOpenCreate = 0x200,
    kSceOpenTruncate = 0x400,
    kNewlibReadOnly = 0,
    kNewlibWriteOnly = 1,
    kNewlibReadWrite = 2,
    kNewlibAppend = 0x8,
    kNewlibCreate = 0x200,
    kNewlibTruncate = 0x400,
    // Permissions offered when creation is requested.
    kCreatePermissions = 0666,
};

// 0x0056ad00
int sceOpen(const char *pszPath, int nFlags) {
    int nMode;

    switch (nFlags & kSceAccessMask) {
    case kSceOpenWriteOnly:
        nMode = kNewlibWriteOnly;
        break;
    case kSceOpenReadWrite:
        nMode = kNewlibReadWrite;
        break;
    default:
        // Read-only and any unexpected mode open for reading.
        nMode = kNewlibReadOnly;
        break;
    }
    if ((nFlags & kSceOpenAppend) != 0) {
        nMode |= kNewlibAppend;
    }
    if ((nFlags & kSceOpenCreate) != 0) {
        nMode |= kNewlibCreate;
    }
    if ((nFlags & kSceOpenTruncate) != 0) {
        nMode |= kNewlibTruncate;
    }
    return open(pszPath, nMode, kCreatePermissions);
}

// 0x0056af88
int sceClose(int nDescriptor) {
    return close(nDescriptor);
}

// 0x0056b340
int sceRead(int nDescriptor, void *pBuffer, int nBytes) {
    return (int)read(nDescriptor, pBuffer, (size_t)nBytes);
}

// 0x0056b5b0
int sceWrite(int nDescriptor, const void *pBuffer, int nBytes) {
    return (int)write(nDescriptor, pBuffer, (size_t)nBytes);
}

// 0x0056b108
int sceLseek(int nDescriptor, int nOffset, int nWhence) {
    // The Sony origins match the newlib ones, so the value passes through.
    return (int)lseek(nDescriptor, (off_t)nOffset, nWhence);
}

// The initialised flag the reset clears, at 0x00762c08 in the image.
static int g_nFsInitialised;

// The cached client word the reset zeroes, at 0x008e3be8 in the image.
static unsigned int g_nFsClient;

// 0x0056acc8
int sceFsReset(void) {
    // The binary clears the flag above and zeroes the client word, then reports success.
    g_nFsInitialised = 0;
    memset(&g_nFsClient, 0, sizeof(g_nFsClient));
    return 0;
}
