#include <eekernel.h>
#include <libcconsole.h>
#include <sifdev.h>
#include <sifdma.h>

// 0x005bc850
// Reports whether the IOP has finished booting. The console is closed once it has, and the next
// console write opens it again.
int sceSifSyncIop(void) {
    if ((sceSifGetReg(SIF_REG_SMFLAG) & SIF_STAT_BOOTEND) == 0) {
        return 0;
    }
    LibcConsoleReset();
    return 1;
}
