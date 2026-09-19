#include "synth/midi_main.h"

#include "os/log.h"

// Selector SynthCommand's third command submits.
constexpr int kSynthSelectorCommandTwo = 0xd0;

// 0x00464ad0. The command number stays in its second argument register from entry so that the
// report below can print it.
void SynthCommand(int nCommand) {
    switch (nCommand) {
    case 0:
        break;
    case 1:
        DumpSynthVoices(1);
        break;
    case 2:
        SubmitSoundDriverRequest(kSynthSelectorCommandTwo, nullptr);
        break;
    default:
        LogPrintf("Unrecognized synth cmd %d\n", nCommand);
        break;
    }
}
