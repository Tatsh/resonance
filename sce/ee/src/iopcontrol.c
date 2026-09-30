#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <libcconsole.h>
#include <sifcmd.h>
#include <sifdev.h>
#include <sifrpc.h>

// The argument prefix that makes the IOP boot loader load a replacement image.
static const char kUdnlPrefix[] = "rom0:UDNL ";
static const char kTooLongFormat[] = "too long parameter '%s'\n";

// 0x008e4a80. SIF DMA needs the 64-byte alignment retail gives it.
static sceSifCmdResetData g_resetPacket __attribute__((aligned(64)));

// 0x005bc6e8
// The argument is copied without its terminator and without a length check.
int sceSifResetIop(const char *arg, int mode) {
    sceSifDmaData transfer;
    unsigned int nIopBuffer;
    int nLength;

    sceSifStopDma();
    nIopBuffer = (unsigned int)sceSifGetReg(SIF_SYSREG_SUBADDR);
    g_resetPacket.flag = mode;
    for (nLength = 0; arg[nLength] != '\0'; ++nLength) {
        g_resetPacket.arg[nLength] = arg[nLength];
    }
    g_resetPacket.chdr.daddr = 0;
    g_resetPacket.size = nLength;
    g_resetPacket.chdr.fcode = SIF_CMDC_RESET_CMD;
    g_resetPacket.chdr.dsize = 0;
    g_resetPacket.chdr.psize = sizeof(g_resetPacket);
    transfer.data = (unsigned int)(uintptr_t)&g_resetPacket;
    transfer.addr = nIopBuffer;
    transfer.size = sizeof(g_resetPacket);
    transfer.mode = SIF_DMA_INT_O | SIF_DMA_ERT;
    sceSifWriteBackDCache(&g_resetPacket, sizeof(g_resetPacket));
    sceSifSetReg(SIF_REG_SMFLAG, SIF_STAT_BOOTEND);
    if (sceSifSetDma(&transfer, 1) == 0) {
        return 0;
    }
    sceSifSetReg(SIF_REG_SMFLAG, SIF_STAT_SIFINIT);
    sceSifSetReg(SIF_REG_SMFLAG, SIF_STAT_CMDINIT);
    sceSifSetReg(SIF_SYSREG_RPCINIT, 0);
    sceSifSetReg(SIF_SYSREG_SUBADDR, 0);
    return 1;
}

// 0x005bc828
int sceSifIsAliveIop(void) {
    return (sceSifGetReg(SIF_REG_SMFLAG) & SIF_STAT_SIFINIT) != 0;
}

// 0x005bc850
int sceSifSyncIop(void) {
    if ((sceSifGetReg(SIF_REG_SMFLAG) & SIF_STAT_BOOTEND) == 0) {
        return 0;
    }
    LibcConsoleReset();
    return 1;
}

// 0x005bc888
int sceSifRebootIop(const char *imgname) {
    char szArg[SIF_CMD_RESET_ARG_MAX];

    if (strlen(imgname) + sizeof(kUdnlPrefix) > SIF_CMD_RESET_ARG_MAX) {
        printf(kTooLongFormat, imgname);
        return 0;
    }
    sceSifInitRpc(0);
    sceSifExitRpc();
    strcpy(szArg, kUdnlPrefix);
    strcat(szArg, imgname);
    return sceSifResetIop(szArg, 0);
}
