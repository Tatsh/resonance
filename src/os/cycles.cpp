#include "os/cycles.h"

unsigned ReadCycleCount() {
    unsigned nCount;
    // The EE keeps its free-running cycle counter in coprocessor 0 register 9.
    asm volatile("mfc0 %0, $9" : "=r"(nCount));
    return nCount;
}
