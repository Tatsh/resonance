#pragma once

/**
 * Voice and sound-bank driver that Ps2HardSynth is a thin class over.
 *
 * Titled after `midi_main.cpp`, the file its own asserts record at `0x0046456c`, `0x00464d58`, and
 * `0x00464ed0`. The module spans `0x004620b0` through `0x00465100` and has the attested behaviour
 * of the sound subsystem: the voice table, the sound banks, and the script-facing command
 * dispatcher. Neither Synth nor Ps2HardSynth has any of it.
 *
 * Recovery has started at the dispatcher and the entry points MainLoop already drives. Three
 * further routines are identified. The bank loaders at `0x00464430` and `0x00464d10` have no
 callers
 * at all: they are async completion callbacks, installed as function pointers, which is why each
 * reads `t1` at entry. That is where the asynchronous layer puts the completion status, and it is
 * the status each reports through `BD bank loading returned async error %d` and `HD bank loading
 * returned async error %d`. Their remaining parameters belong to the callback type in
 * `os/async.h`, which another subsystem owns, so they stay undeclared here.

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
 * selectors are 0xd0 from SynthCommand(), and 0x1070 and 0x1050 from the bank loader.
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
 * Report every active voice to the log.
 *
 * Writes one `Voice %2.2d at %x env %x - end %d mix %d %d %d %d` line per voice and then
 * `Using %d voices total`. SynthCommand() passes 1.
 *
 * @param nDetail Retained on the stack and otherwise unrecovered.
 * @ghidraAddress 0x00462558
 */
void DumpSynthVoices(int nDetail);

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
