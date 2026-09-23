#include "app/longop.h"

// 0x00719858
LongOperationProc g_pfnLongOperationPollProc;
// 0x0071985c
LongOperationProc g_pfnLongOperationDrawProc;

// 0x00520428
void SetLongOperationPollProc(LongOperationProc pfnPoll) {
    g_pfnLongOperationPollProc = pfnPoll;
}

// 0x00520438
void RunLongOperationPollProc() {
    if (g_pfnLongOperationPollProc != nullptr) {
        g_pfnLongOperationPollProc();
    }
}

// 0x00520460
void SetLongOperationDrawProc(LongOperationProc pfnDraw) {
    g_pfnLongOperationDrawProc = pfnDraw;
}

// 0x00520470
void RunLongOperationDrawProc() {
    if (g_pfnLongOperationDrawProc != nullptr) {
        g_pfnLongOperationDrawProc();
    }
}
