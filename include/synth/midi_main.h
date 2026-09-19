#pragma once

#include <libsdr.h>

/**
 * Voice and sound-bank driver that Ps2HardSynth is a thin class over.
 *
 * Titled after `midi_main.cpp`, the string at `0x0081cc98` that the module bills its heap releases
 * to. The module spans `0x00461f28` through `0x00465200` and has the attested behaviour
 * of the sound subsystem: the SPU2 voices, the sound banks, and the script-facing command
 * dispatcher. Neither Synth nor Ps2HardSynth has any of it.
 *
 * There is no voice table. Every routine here that reports or configures a voice queries the SPU2
 * itself through libsdr's remote-call trampoline, `sceSdRemote()`, with the `rSd*` function code
 * and the `SD_*` entry encoding inline at the call site. The image has no `sceSd*` wrapper
 * function, so the SDK release of this era supplied the whole API as macros over that one entry
 * point. Correspondence is exact at both levels: the function codes are `rSdInit`, `rSdSetParam`,
 * `rSdGetParam`, `rSdGetSwitch`, `rSdSetAddr`, `rSdGetAddr`, `rSdSetCoreAttr`, `rSdGetCoreAttr` and
 * `rSdSetEffectAttr`, and the entries carry libsd's `0x80` core-level flag on exactly the
 * core-level registers.
 *
 * The two bank transfers are classes rather than routines. CallbackXferBdToIop streams a BD bank in
 * 0x2000-byte chunks and CallbackXferHdToIop moves an HD bank in one go, and both derive from
 * AsyncCallback. The globals below are what the two share with the rest of the module.
 */

/**
 * Run one script-facing synth command.
 *
 * Three command numbers are recognised. Command 0 does nothing. Command 1 reports the voices
 * through DumpSynthVoices(). Command 2 submits driver selector 0xd0 with no command block. Anything
 * else reports `Unrecognized synth cmd %d`, and the command number is retained in its second
 * argument register from entry so that the report can print it.
 *
 * The script layer exposes this as `synth_cmd`, which is the one attested title in the module.
 *
 * @param nCommand The command to run.
 * @ghidraAddress 0x00464ad0
 */
void SynthCommand(int nCommand);

/**
 * Submit one command to the sound driver.
 *
 * Every routine in the module funnels through this, twenty call sites in all, each passing a
 * selector word and either a command block or nothing. The selector is a bit field: the routine
 * tests bits 0x8000 and 0x1000 of it, spins on a semaphore, and then hands the block on. Observed
 * selectors are 0xd0 from SynthCommand() and from the tail of DumpSynthVoices(), 0x1070 and 0x1050
 * from the bank loader, 0x10e0 from ConfigureSpu2Effects(), and 0x8130 from `0x004642c8`.
 *
 * The title comes from what the twenty call sites have in common rather than from any one of them.
 * The block's shape varies by selector, so it is opaque here; a null block is valid.
 *
 * @param nSelector The command selector.
 * @param pCommand The command block, or null.
 * @ghidraAddress 0x005f96c8
 */
void SubmitSoundDriverRequest(int nSelector, void *pCommand);

/**
 * Move a buffer from main memory into the sound driver's memory on the IOP.
 *
 * Fills the descriptor at `0x008e5be8` with the three arguments and a zero fourth word, then issues
 * the transfer and spins until it reports completion.
 *
 * @param nIopAddress The destination on the IOP.
 * @param pSource The source in main memory.
 * @param nLength The number of bytes to move.
 * @return Zero once the transfer has completed, or -1 when it could not be started.
 * @ghidraAddress 0x005f97d0
 */
int XferToIop(int nIopAddress, const void *pSource, int nLength);

/** Bytes the chunk command block clears above its five recovered words. */
constexpr int kSynthXferCommandTailSize = 0x6c;

/**
 * Sound-driver command block that describes one chunk of a sound bank.
 *
 * Both bank-transfer paths fill the same single block and submit it under selector 0x1070. The
 * first word is cleared before every submission and read nowhere in the image, and the tail is
 * cleared with it, so the block is 0x80 bytes in total.
 */
struct SynthXferCommand {
    int mUnknown00;  // +0x00 cleared before every submission
    int mIopAddress; // +0x04 staging buffer on the IOP the chunk was moved to
    int mLength;     // +0x08
    int mDest;       // +0x0c where the driver writes the chunk, advanced one chunk at a time
    int mUnknown10;  // +0x10 copied from g_nSynthXferTag
    char mUnused14[kSynthXferCommandTailSize]; // +0x14 cleared before every submission
};

/**
 * Block that describes one chunk of a sound bank.
 *
 * @ghidraAddress 0x00894cc0
 */
extern SynthXferCommand g_synthXferCommand;

/** Staging buffers on the IOP that chunk transfers alternate between. */
constexpr int kIopStagingBufferCount = 2;

/**
 * Addresses of the staging buffers on the IOP.
 *
 * The table itself may be longer. Only the first two entries are reachable, because the index below
 * is masked to one bit.
 *
 * @ghidraAddress 0x00894748
 */
extern int g_anIopStagingAddress[kIopStagingBufferCount];

/**
 * Staging buffer the next chunk transfer will use.
 *
 * @ghidraAddress 0x006e9b80
 */
extern int g_nIopStagingIndex;

/**
 * Address on the IOP a sound bank is moved to.
 *
 * CallbackXferHdToIop::Done() moves the whole bank there, and the routine that starts a BD transfer
 * copies it into the first word of the bank-complete block.
 *
 * @ghidraAddress 0x006e9b84
 */
extern int g_nBankIopAddress;

/**
 * Word every chunk command block is stamped with at `+0x10`.
 *
 * What the driver does with it is unrecovered, and the writer has not been identified.
 *
 * @ghidraAddress 0x006e9bb4
 */
extern int g_nSynthXferTag;

/**
 * Called once after every chunk is submitted and once when a bank transfer finishes.
 *
 * Both call sites test the hook against null first. It takes no arguments.
 *
 * @ghidraAddress 0x006e9bc4
 */
extern void (*g_pfnBankLoadProgress)();

/**
 * Install the bank-load progress hook.
 *
 * @param pfnProgress The hook, which may be null.
 * @ghidraAddress 0x00464378
 */
void SetBankLoadProgressHook(void (*pfnProgress)());

/**
 * Non-zero while an HD bank transfer occupies the shared command block.
 *
 * CallbackXferBdToIop defers to it, and CallbackXferHdToIop::Done() clears it. The routine that
 * sets it is unrecovered.
 *
 * @ghidraAddress 0x006e9dc8
 */
extern int g_nHdXferInFlight;

/**
 * Block the bank-complete report submits.
 *
 * Its shape is unrecovered. The routine that fills it, at `0x00461f28`, starts a transfer and is
 * not reconstructed, and the five words it writes do not line up with SynthXferCommand.
 *
 * @ghidraAddress 0x00894bc0
 */
extern char g_abBankCompleteCommand[];

/**
 * Buffer the HD bank transfer read into, released once the transfer reports.
 *
 * @ghidraAddress 0x006e9dcc
 */
extern void *g_pHdXferBuffer;

/**
 * Buffer the BD bank transfer reads into, released once the last chunk has been moved.
 *
 * This is the allocation as it came back from the heap. The read itself uses the pointer rounded up
 * to a 64-byte boundary, which CallbackXferBdToIop stores separately.
 *
 * @ghidraAddress 0x006e9dd0
 */
extern void *g_pBdXferBuffer;

/**
 * Report the state of both SPU2 cores and all 48 voices to the log.
 *
 * Writes one `Core    %2.2d: MMix %x eff %d vmix L %x %x R %x %x end %x` line per core, one
 * `  Voice %2.2d at %x env %x - end %d mix %d %d %d %d` line per voice, and finally
 * `Using %d voices total`. Every value is read back from the hardware, so the report is the live
 * mixer state rather than anything the driver retains. A voice counts towards the total when its
 * ENDX bit is clear, which is the sample not having run out.
 *
 * @param bActiveOnly Non-zero to report only the voices whose ENDX bit is clear. SynthCommand()
 *                    passes 1, and zero reports all 24 voices of each core.
 * @ghidraAddress 0x00462558
 */
void DumpSynthVoices(int bActiveOnly);

/**
 * Bind libsdr and bring both SPU2 cores up.
 *
 * Sets the two mixer routings, silences the effect send, opens both master volumes fully, and gives
 * each core a 128 KB effect work area at the top of sound RAM, core 0 taking the highest block.
 * Core 1's mixer routing takes the external input that core 0's does not, which is what chains the
 * two cores.
 *
 * InitSynthDriver() is the only caller.
 *
 * @ghidraAddress 0x004649f8
 */
void InitSpu2Cores();

/**
 * Apply the configured reverb to both SPU2 cores.
 *
 * Per core, the routine enables the effect only when the argument is non-zero and the configuration
 * also enables it for that core; otherwise it clears both effect volumes and turns the core
 * attribute off. Either way it then reopens both master volumes fully. When effects are enabled it
 * fills a `sceSdEffectAttr` from five configuration values, ORs `SD_EFFECT_MODE_CLEAR` into the
 * mode, and both depths are the configured value shifted left by eight.
 *
 * Enabling also submits a further command block to the sound driver under selector 0x10e0, built
 * from eleven more configuration values.
 *
 * The body is not reconstructed. It reads every value through the two variadic configuration
 * queries at `0x00509110` and `0x005093e0`, whose signatures belong to the data-array layer.
 *
 * @param bEnable Zero to force the effect off on both cores.
 * @ghidraAddress 0x00462340
 */
void ConfigureSpu2Effects(int bEnable);

/**
 * Bring the voice and bank driver up.
 *
 * CreatePs2HardSynth() calls this immediately after construction, which is the seam between the
 * Synth class and this module.
 *
 * @ghidraAddress 0x004647a8
 */
void InitSynthDriver();

/**
 * Advance the streaming voice buffers.
 *
 * Performs no work while the stream at `0x006f9c58` is absent. MainLoop::Poll() drives this once
 * per frame.
 *
 * @ghidraAddress 0x00464bc8
 */
void PollSynthStream();

/**
 * Advance the driver's pending events.
 *
 * One of MainLoop's two periodic timers drives this.
 *
 * @ghidraAddress 0x004648c8
 */
void PollSynthEvents();
