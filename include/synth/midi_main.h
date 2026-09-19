#pragma once

#include <libsdr.h>

/**
 * Voice and sound-bank driver that Ps2HardSynth is a thin class over.
 *
 * Titled after `midi_main.cpp`, the file its own asserts record at `0x0046456c`, `0x00464d58`, and
 * `0x00464ed0`. The module spans `0x004620b0` through `0x00465100` and has the attested behaviour
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
 * The two bank loaders at `0x00464430` and `0x00464d10` are virtual overrides, not free callbacks.
 * The RTTI gives their classes as `CallbackXferBdToIop` and `CallbackXferHdToIop`, each with
 * `AsyncCallback` as its one public base at offset 0, which is why each reads a register no
 * ordinary call would pass: the argument list is the base's completion signature. Each reports its
 * failure through `BD bank loading returned async error %d` or `HD bank loading returned async
 * error %d`. Both stay undeclared until `AsyncCallback` is declared, which belongs to the
 * asynchronous layer rather than here.
 *
 * The BD loader is a chunked streaming read. It marks the request busy, fills a command block from
 * the request, submits it, then re-arms the asynchronous read for the next 0x2000 bytes until the
 * remaining count falls to zero, at which point it closes the file and notifies a completion hook.
 */

/**
 * Run one script-facing synth command.
 *
 * Three command numbers are recognised. Command 0 does nothing. Command 1 reports the voice table
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
 * One chunked sound-bank read in flight.
 *
 * The layout comes from the BD bank loader at `0x00464430`, which is the only routine recovered
 * that touches every field. The title is inferred from what that routine does, because no string in
 * the image identifies the structure. The first word is not read by the loader, and the structure
 * is at least 0x20 bytes.
 */
struct SynthBankLoad {
    int mUnknown00;   // +0x00
    int mRequestId;   // +0x04 asynchronous request the next chunk was submitted under
    int mFile;        // +0x08 closed once the remaining count falls to zero
    int mUnknown0c;   // +0x0c passed to the read and to `0x005f97d0`
    int mChunkLength; // +0x10 bytes in the chunk just completed, 0x2000 until the last one
    char *mDest;      // +0x14 advanced by mChunkLength per chunk
    int mRemaining;   // +0x18 bytes still to read
    int mBusy;        // +0x1c set while a chunk is in flight, cleared when idle
};

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
