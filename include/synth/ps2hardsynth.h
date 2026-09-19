#pragma once

#include "synth/midi_main.h"
#include "synth/synth.h"

/**
 * The game's hardware synthesiser.
 *
 * `12Ps2HardSynth` in the RTTI descriptor at `0x009021d0`, with Synth as its one base, so the
 * inherited MsgSink vptr sits at offset 0 and the class is 0xc bytes. Its vtable is at
 * `0x00814fc0` and has eighteen slots, one more than the seventeen Synth declares, so the class
 * adds a single virtual of its own at slot 17 (`0x003f4590`).
 *
 * It answers ten of Synth's thirteen interface slots and inherits slots 7, 10, and 16 unchanged.
 * Which slot means what is recorded in `synth/synth.h` rather than guessed at here, because no
 * string in the image identifies a member of the interface.
 *
 * The class is thin. The attested work happens in the driver at `0x00462558` through `0x00465000`,
 * which owns the voices and the sound banks, and CreateSynth() hands off to that driver's
 * initialiser at `0x004647a8` immediately after construction. The two members below are the only
 * state, and neither is read by anything recovered so far.
 *
 * Globals owns the single instance, created in Globals::CreateSynth() and destroyed in
 * Globals::Shutdown().
 */
class Ps2HardSynth : public Synth {
public:
    /**
     * @ghidraAddress 0x003f6670
     */
    virtual ~Ps2HardSynth();

private:
    int mUnknown04; // +0x04 set to 1 on construction
    int mUnknown08; // +0x08 cleared on construction
};

/**
 * Create the hardware synthesiser.
 *
 * Takes 0xc bytes against the tag at `0x00814e90`, sets `+0x04` to 1 and clears `+0x08`, then
 * brings the driver up through `0x004647a8`.
 *
 * @return The new synthesiser.
 * @ghidraAddress 0x003f4db8
 */
Ps2HardSynth *CreatePs2HardSynth();
