#include "os/cycles.h"

// The hardware path. The EE keeps its free-running cycle counter in coprocessor 0 register 9, and
// the header records why the image has no out-of-line copy of this read. A port supplies its own
// file for this one definition and shares everything else here.
unsigned ReadCycleCount() {
    unsigned nCount;
    asm volatile("mfc0 %0, $9" : "=r"(nCount));
    return nCount;
}

// The clock state the inline reader in the header accumulates into. Around thirty routines across
// the engine reach these three words directly, which is what makes them engine-wide rather than
// private to this file.

long long g_llTotalCycles;

unsigned g_nLastCycleCount;

unsigned g_nLastCycleDelta;
