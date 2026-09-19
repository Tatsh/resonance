#include "synth/midi_main.h"

#include <libsdr.h>

#include "os/log.h"

// Selector submitted by SynthCommand's third command and by the tail of the voice report. What it
// asks the driver to do is unrecovered, so the title records the selector rather than an effect.
constexpr int kSoundSelectorUnknownD0 = 0xd0;

// libsdr's trampoline routes a zero first argument through its callback thread. Every call site in
// the game passes 1, which is the SDK's own `blocking` argument.
constexpr int kSdRemoteBlocking = 1;

constexpr int kSpu2CoreCount = 2;

constexpr int kSpu2VoicesPerCore = 24;

// Both voice-address entries reach libsd with this bit set above the 0x3e voice field. The library
// masks the voice out, so the bit does not change which voice is read; the SDK of this era
// evidently folded it into the two SD_VADDR_* constants the way it folds 0x80 into the core-level
// SD_PARAM_* ones.
constexpr int kSdVoiceAddrEntryBit = 0x40;

// Mixer routing each core starts with. Core 1 additionally takes the input core 0 feeds it.
constexpr int kSpu2Core0Mix = 0xf00;
constexpr int kSpu2Core1Mix = 0xfcc;

constexpr int kSpu2MaxVolume = 0x3fff;

// Highest byte of sound RAM, which is where core 0's effect work area ends. Each core is given the
// block below the previous one.
constexpr unsigned kSpu2EffectAreaTop = 0x1fffff;
constexpr unsigned kSpu2EffectAreaSize = 0x20000;

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
        SubmitSoundDriverRequest(kSoundSelectorUnknownD0, nullptr);
        break;
    default:
        LogPrintf("Unrecognized synth cmd %d\n", nCommand);
        break;
    }
}

// 0x00462558
void DumpSynthVoices(int bActiveOnly) {
    int nActive = 0;
    for (int nCore = 0; nCore < kSpu2CoreCount; ++nCore) {
        const int nMMix = sceSdRemote(kSdRemoteBlocking, rSdGetParam, SD_PARAM_MMIX | nCore);
        const int nEffect =
            sceSdRemote(kSdRemoteBlocking, rSdGetCoreAttr, SD_CORE_EFFECT_ENABLE | nCore);
        const int nVMixL = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_SWITCH_VMIXL | nCore);
        const int nVMixEL = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_SWITCH_VMIXEL | nCore);
        const int nVMixR = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_SWITCH_VMIXR | nCore);
        const int nVMixER = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_SWITCH_VMIXER | nCore);
        const int nEndX = sceSdRemote(kSdRemoteBlocking, rSdGetSwitch, SD_SWITCH_ENDX | nCore);
        LogPrintf("Core    %2.2d: MMix %x eff %d vmix L %x %x R %x %x end %x\n",
                  nCore,
                  nMMix,
                  nEffect,
                  nVMixL,
                  nVMixEL,
                  nVMixR,
                  nVMixER,
                  nEndX);
        for (int nVoice = 0; nVoice < kSpu2VoicesPerCore; ++nVoice) {
            const int nEntry = SD_VOICE(nCore, nVoice);
            const int nStart = sceSdRemote(
                kSdRemoteBlocking, rSdGetAddr, SD_VADDR_SSA | kSdVoiceAddrEntryBit | nEntry);
            // Yes, the binary discards this read's result.
            sceSdRemote(
                kSdRemoteBlocking, rSdGetAddr, SD_VADDR_NAX | kSdVoiceAddrEntryBit | nEntry);
            const int nEnvX = sceSdRemote(kSdRemoteBlocking, rSdGetParam, SD_VPARAM_ENVX | nEntry);
            const int nBit = 1 << nVoice;
            if ((nEndX & nBit) == 0) {
                ++nActive;
            } else if (bActiveOnly) {
                continue;
            }
            LogPrintf("  Voice %2.2d at %x env %x - end %d mix %d %d %d %d\n",
                      nVoice,
                      nStart,
                      nEnvX,
                      (nEndX & nBit) != 0,
                      (nVMixL & nBit) != 0,
                      (nVMixEL & nBit) != 0,
                      (nVMixR & nBit) != 0,
                      (nVMixER & nBit) != 0);
        }
    }
    SubmitSoundDriverRequest(kSoundSelectorUnknownD0, nullptr);
    LogPrintf("Using %d voices total\n", nActive);
}

// 0x004649f8
void InitSpu2Cores() {
    sceSdRemoteInit(); // Yes, the binary discards this call's result.
    sceSdRemote(kSdRemoteBlocking, rSdInit, 0);
    sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_PARAM_MMIX | 1, kSpu2Core1Mix);
    sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_PARAM_MMIX | 0, kSpu2Core0Mix);
    unsigned nEffectEnd = kSpu2EffectAreaTop;
    for (int nCore = 0; nCore < kSpu2CoreCount; ++nCore) {
        sceSdRemote(kSdRemoteBlocking, rSdSetCoreAttr, SD_CORE_EFFECT_ENABLE | nCore, 0);
        sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_PARAM_MVOLL | nCore, kSpu2MaxVolume);
        sceSdRemote(kSdRemoteBlocking, rSdSetParam, SD_PARAM_MVOLR | nCore, kSpu2MaxVolume);
        sceSdRemote(kSdRemoteBlocking, rSdSetAddr, SD_ADDR_EEA | nCore, nEffectEnd);
        nEffectEnd -= kSpu2EffectAreaSize;
    }
}
