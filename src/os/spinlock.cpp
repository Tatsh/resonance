#include "os/spinlock.h"

// The interrupt enable bit in the coprocessor 0 status register.
constexpr unsigned int kStatusInterruptBit = 0x10000u;

// 0x005e4510
bool SpinDisableInterrupts(void) {
    unsigned int nStatus;

    __asm__ volatile("mfc0 %0, $12" : "=r"(nStatus));
    if ((nStatus & kStatusInterruptBit) == 0) {
        return false;
    }
    do {
        __asm__ volatile("di");
        __asm__ volatile("sync 0x10");
        __asm__ volatile("mfc0 %0, $12" : "=r"(nStatus));
    } while ((nStatus & kStatusInterruptBit) != 0);
    return true;
}

// 0x005e4558
bool ReenableInterrupts(void) {
    unsigned int nStatus;

    __asm__ volatile("mfc0 %0, $12" : "=r"(nStatus));
    __asm__ volatile("ei");
    return (nStatus & kStatusInterruptBit) != 0;
}
