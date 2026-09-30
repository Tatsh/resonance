#include <kernel.h>
#include <sif.h>
#include <sifrpc.h>

#include "ezmidi/ezmidi.h"

enum {
    kMidiThreadStackSize = 2048,
    kMidiThreadPriority = 30,
};

// 0x6e60
ModuleInfo Module = {"ezmidi_driver", 0x0103};

// 0x0000
int start(void) {
    struct ThreadParam param;
    int th;

    CpuEnableIntr();
    if (!sceSifCheckInit()) {
        sceSifInit();
    }
    sceSifInitRpc(0);

    param.attr = TH_C;
    param.entry = (void (*)(void))sce_midi_loop;
    param.initPriority = kMidiThreadPriority;
    param.stackSize = kMidiThreadStackSize;
    param.option = 0;
    th = CreateThread(&param);
    if (th > 0) {
        StartThread(th, 0);
        return 0;
    }
    return 1;
}
