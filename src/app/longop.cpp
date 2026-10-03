#include "app/longop.h"

// NTSC-U/C: 0x00719858, PAL: 0x0075d758
LongOperationProc g_pfnLongOperationPollProc;
// NTSC-U/C: 0x0071985c, PAL: 0x0075d75c
LongOperationProc g_pfnLongOperationDrawProc;

// NTSC-U/C: 0x00520428, PAL: 0x00560980
void SetLongOperationPollProc(LongOperationProc pfnPoll) {
    g_pfnLongOperationPollProc = pfnPoll;
}

// NTSC-U/C: 0x00520438, PAL: 0x00560990
void RunLongOperationPollProc() {
    if (g_pfnLongOperationPollProc != nullptr) {
        g_pfnLongOperationPollProc();
    }
}

// NTSC-U/C: 0x00520460, PAL: 0x005609b8
void SetLongOperationDrawProc(LongOperationProc pfnDraw) {
    g_pfnLongOperationDrawProc = pfnDraw;
}

// NTSC-U/C: 0x00520470, PAL: 0x005609c8
void RunLongOperationDrawProc() {
    if (g_pfnLongOperationDrawProc != nullptr) {
        g_pfnLongOperationDrawProc();
    }
}
