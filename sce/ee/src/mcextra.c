#include <libmc.h>
#include <stddef.h>

enum {
    // The create flag OpenWriteOp ORs into its open mode at 0x0055ebe8. The call passes
    // 0x202, which is this flag combined with the writeable attribute (0x002).
    kMcFileCreateFile = 0x200,
    // The shared send and receive area holds this many 32-bit words (0x80 bytes).
    kMcSlotMaxPacketWords = 0x20,
    // The slot-count query is function one, sent with no mode bits.
    kMcSlotMaxFunction = 1,
    kMcSlotMaxMode = 0,
    // Both the send and the receive transfer this many bytes.
    kMcSlotMaxTransferSize = 0x80,
    // The client area covers the fields the remote call writes.
    kMcSlotMaxClientSize = 0x40,
};

// The diagnostic text the failure path passes to the stub.
static const char *const kMcSlotMaxErrorText = (const char *)0x00828a38;

// The shared send and receive area for the query, at 0x0089e180 in the image.
static unsigned int g_mcSlotMaxPacket[kMcSlotMaxPacketWords];

// The client area the query binds, at 0x0089e0d0 in the image.
static unsigned char g_mcSlotMaxClient[kMcSlotMaxClientSize];

// 0x005652c8
extern int sceSifCallRpcInternal(void *pClient,
                                 int nFunction,
                                 int nMode,
                                 void *pSend,
                                 int nSendSize,
                                 void *pReceive,
                                 int nReceiveSize,
                                 void *pExtra,
                                 int nReserved);

// 0x0053a598
static void reportMcError(const char *pszFormat, ...) {
    // The shipped stub spills its arguments and returns without printing.
    (void)pszFormat;
}

// 0x0053a920
int sceMcGetSlotMax(int nPort) {
    g_mcSlotMaxPacket[0] = (unsigned int)nPort;
    if (sceSifCallRpcInternal(g_mcSlotMaxClient,
                              kMcSlotMaxFunction,
                              kMcSlotMaxMode,
                              g_mcSlotMaxPacket,
                              kMcSlotMaxTransferSize,
                              g_mcSlotMaxPacket,
                              kMcSlotMaxTransferSize,
                              NULL,
                              0) < 0) {
        reportMcError(kMcSlotMaxErrorText);
        return 0;
    }
    return (int)g_mcSlotMaxPacket[1];
}
