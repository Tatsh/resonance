#ifndef LIBKERNELINTERNAL_H
#define LIBKERNELINTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/** The system calls and data the kernel library uses without exporting them to the game. */

/** Sizes of the alarm patch images the library copies into the kernel. */
enum {
    kAlarmPatchSize = 1856,    /*!< Size of g_alarmPatch in bytes. */
    kAlarmTrampolineSize = 40, /*!< Size of g_alarmTrampoline in bytes. */
};

/**
 * The alarm system calls, built to run at kernel address 0x80076000.
 *
 * @ghidraAddress NTSC-U/C: 0x007690b8
 * @ghidraAddress PAL: 0x007ace10
 */
extern const unsigned char g_alarmPatch[kAlarmPatchSize];

/**
 * The user-mode trampoline an alarm handler runs through, built to run at 0x00082000.
 *
 * @ghidraAddress NTSC-U/C: 0x007697f8
 * @ghidraAddress PAL: 0x007ad550
 */
extern const unsigned char g_alarmTrampoline[kAlarmTrampolineSize];

/**
 * System call 20. Unmasks an interrupt cause.
 *
 * @param cause Interrupt cause.
 * @return Kernel result.
 * @ghidraAddress NTSC-U/C: 0x00536820
 * @ghidraAddress PAL: 0x005760e0
 */
int _EnableIntc(int cause);

/**
 * System call 21. Masks an interrupt cause.
 *
 * @param cause Interrupt cause.
 * @return Kernel result.
 * @ghidraAddress NTSC-U/C: 0x00536830
 * @ghidraAddress PAL: 0x005760f0
 */
int _DisableIntc(int cause);

/**
 * System call 22. Unmasks a DMA channel interrupt.
 *
 * @param channel DMA channel.
 * @return Kernel result.
 * @ghidraAddress NTSC-U/C: 0x00536840
 * @ghidraAddress PAL: 0x00576100
 */
int _EnableDmac(int channel);

/**
 * System call 23. Masks a DMA channel interrupt.
 *
 * @param channel DMA channel.
 * @return Kernel result.
 * @ghidraAddress NTSC-U/C: 0x00536850
 * @ghidraAddress PAL: 0x00576110
 */
int _DisableDmac(int channel);

/**
 * System call -52. Wakes a thread from an interrupt handler, other than the interrupted thread.
 *
 * @param thid Thread identifier.
 * @return Kernel result.
 * @ghidraAddress NTSC-U/C: 0x00536a20
 * @ghidraAddress PAL: 0x005762e0
 */
int _iWakeupThread(int thid);

#ifdef __cplusplus
}
#endif

#endif
