#include <stdint.h>

#include <sifdev.h>

// The SIF DMA and register calls are kernel calls. A negative number selects the form that is safe
// in an interrupt handler.
enum {
    kSyscallSifStopDma = 107,
    kSyscallSifDmaStat = 118,
    kSyscallSifSetDma = 119,
    kSyscallSifSetDChain = 120,
    kSyscallSifSetReg = 121,
    kSyscallSifGetReg = 122,
};

// Issue kernel call nNumber with two argument words and return the result word. The kernel may
// change every register the calling convention does not preserve.
static inline int sifSyscall(int nNumber, int nFirstArgument, int nSecondArgument) {
    register int v0 __asm__("$2");
    register int v1 __asm__("$3") = nNumber;
    register int a0 __asm__("$4") = nFirstArgument;
    register int a1 __asm__("$5") = nSecondArgument;

    __asm__ volatile("syscall"
                     : "=r"(v0), "+r"(v1), "+r"(a0), "+r"(a1)
                     :
                     : "$1",
                       "$6",
                       "$7",
                       "$8",
                       "$9",
                       "$10",
                       "$11",
                       "$12",
                       "$13",
                       "$14",
                       "$15",
                       "$24",
                       "$25",
                       "hi",
                       "lo",
                       "memory");
    return v0;
}

// NTSC-U/C: 0x00536db0, PAL: 0x00576670
void sceSifStopDma(void) {
    sifSyscall(kSyscallSifStopDma, 0, 0);
}

// NTSC-U/C: 0x00536e80, PAL: 0x00576740
int sceSifDmaStat(unsigned int id) {
    return sifSyscall(kSyscallSifDmaStat, (int)id, 0);
}

// NTSC-U/C: 0x00536e90, PAL: 0x00576750
int isceSifDmaStat(unsigned int id) {
    return sifSyscall(-kSyscallSifDmaStat, (int)id, 0);
}

// NTSC-U/C: 0x00536ea0, PAL: 0x00576760
unsigned int sceSifSetDma(sceSifDmaData *sdd, int len) {
    return (unsigned int)sifSyscall(kSyscallSifSetDma, (int)(uintptr_t)sdd, len);
}

// NTSC-U/C: 0x00536eb0, PAL: 0x00576770
unsigned int isceSifSetDma(sceSifDmaData *sdd, int len) {
    return (unsigned int)sifSyscall(-kSyscallSifSetDma, (int)(uintptr_t)sdd, len);
}

// NTSC-U/C: 0x00536ec0, PAL: 0x00576780
void sceSifSetDChain(void) {
    sifSyscall(kSyscallSifSetDChain, 0, 0);
}

// NTSC-U/C: 0x00536ed0, PAL: 0x00576790
void isceSifSetDChain(void) {
    sifSyscall(-kSyscallSifSetDChain, 0, 0);
}

// NTSC-U/C: 0x00536ee0, PAL: 0x005767a0
int sceSifSetReg(unsigned int reg, int val) {
    return sifSyscall(kSyscallSifSetReg, (int)reg, val);
}

// NTSC-U/C: 0x00536ef0, PAL: 0x005767b0
int sceSifGetReg(unsigned int reg) {
    return sifSyscall(kSyscallSifGetReg, (int)reg, 0);
}
