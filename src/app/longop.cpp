#include "app/longop.h"

// NTSC-U/C: 0x00719858, PAL: 0x0075d758
LongOperationProc g_pfnLongOperationPollProc;
// NTSC-U/C: 0x0071985c, PAL: 0x0075d75c
LongOperationProc g_pfnLongOperationDrawProc;

void SetLongOperationPollProc(LongOperationProc pfnPoll) {
    g_pfnLongOperationPollProc = pfnPoll;
}

void RunLongOperationPollProc() {
    if (g_pfnLongOperationPollProc != nullptr) {
        g_pfnLongOperationPollProc();
    }
}

void SetLongOperationDrawProc(LongOperationProc pfnDraw) {
    g_pfnLongOperationDrawProc = pfnDraw;
}

void RunLongOperationDrawProc() {
    if (g_pfnLongOperationDrawProc != nullptr) {
        g_pfnLongOperationDrawProc();
    }
}
