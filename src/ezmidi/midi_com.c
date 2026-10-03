#include <stddef.h>

#include <kernel.h>
#include <sifrpc.h>
#include <stdio.h>

#include "ezmidi/ezmidi.h"
#include "ezmidi/hsyn.h"

enum {
    kAllChannels = -1,
    kRpcArgumentWords = 32,
};

// NTSC-U/C: 0x6e70, PAL: 0x6e70
int ret = 0;

// NTSC-U/C: 0x8570, PAL: 0x8580
int gRpcArg[kRpcArgumentWords];

// NTSC-U/C: 0x018c, PAL: 0x018c
static void *midiFunc(unsigned int command, void *data, int size) {
    EZMIDI_BANK *pBank = NULL;
    int ch = command & EZMIDI_CMD_PORT_MASK;
    int *pArg = data;

    (void)size; // The binary never reads the size.
    ret = 0;
    switch (command & EZMIDI_CMD_MASK) {
    case EZMIDI_CMD_INIT:
        ret = HardSynthInit();
        break;
    case EZMIDI_CMD_ATTACH_HD:
        pBank = data;
        ret = HardSynthAttachHDtoBD(ch, pBank->hdAddr, pBank->spuAddr, pBank->bank);
        break;
    case EZMIDI_CMD_LOAD_BD:
        pBank = data;
        ret = HardSynthLoadBD(pBank->bdAddr, pBank->spuAddr, pBank->bdSize);
        break;
    case EZMIDI_CMD_ALL_NOTES_OFF:
        HardSynthAllNotesOff(kAllChannels, 1);
        break;
    case EZMIDI_CMD_CONFIG:
        HardSynthConfig(data);
        break;
    case EZMIDI_CMD_PAUSE:
        if (*pArg) {
            HardSynthPause();
        } else {
            HardSynthResume();
        }
        break;
    case EZMIDI_CMD_REMIX:
        HardSynthSetRemix(*pArg);
        break;
    case EZMIDI_CMD_MONO:
        HardSynthSetMono(*pArg);
        break;
    case EZMIDI_CMD_INFO:
        HardSynthInfo(*pArg);
        break;
    case EZMIDI_CMD_INVALIDATE_HD:
        HardSynthInvalidateHd((unsigned char *)*pArg);
        break;
    case EZMIDI_CMD_INVALIDATE_BANK:
        ret = HardSynthInvalidateBank(*pArg);
        break;
    default:
        printf("EzMIDI driver error: unknown command %d (data %d) \n", command, *pArg);
        break;
    }
    return &ret;
}

// NTSC-U/C: 0x00d0, PAL: 0x00d0
int sce_midi_loop(void) {
    sceSifQueueData qd;
    sceSifServeData sd;

    CpuEnableIntr();
    EnableIntr(INUM_DMA_4);
    EnableIntr(INUM_DMA_7);
    sceSifInitRpc(0);
    sceSifSetRpcQueue(&qd, GetThreadId());
    sceSifRegisterRpc(&sd, EZMIDI_RPC_SERVER, midiFunc, gRpcArg, NULL, NULL, &qd);
    sceSifRpcLoop(&qd);
    return 0;
}
