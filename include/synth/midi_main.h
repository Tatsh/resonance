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
 * further routines are identified and not declared, each for a stated reason. The bank loaders at
 * `0x00464430` and `0x00464d10`, which report `BD bank loading returned async error %d` and
 * `HD bank loading returned async error %d`, both read `t0` and `t1` at entry, so each takes at
 * least six integer arguments; they stay undeclared until those are typed rather than counted. The
 * routine at `0x005f96c8` that the dispatcher's third command calls is undeclared because it is
 * unidentified: it takes a flags word, testing bits `0x8000` and `0x1000` of it, spins on a
 * semaphore, and sits outside this module entirely, so no title for it follows from the one call
 * site.
 */

/**
 * Run one script-facing synth command.
 *
 * Three command numbers are recognised. Command 0 does nothing. Command 1 reports the voice table
 * through DumpSynthVoices(). Command 2 calls the unidentified routine at `0x005f96c8` with a flags
 * word of 0xd0. Anything else reports `Unrecognized synth cmd %d`, and the command number is
 * retained in its second argument register from entry so that the report can print it.
 *
 * The script layer exposes this as `synth_cmd`, which is the one attested title in the module. The
 * body is not written while command 2's callee has no title, because the only title available for
 * it would be inferred from this call site rather than from the routine.
 *
 * @param nCommand The command to run.
 * @ghidraAddress 0x00464ad0
 */
void SynthCommand(int nCommand);

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
