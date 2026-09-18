#pragma once

/**
 * The game's hardware synthesiser.
 *
 * `12Ps2HardSynth` in the RTTI descriptor at `0x009021d0`, deriving from `5Synth` at `0x008ef430`,
 * which derives in turn from MsgSink. Recovery has barely started. The object is 0x0c bytes, its
 * table is at `0x00814fc0`, and CreatePs2HardSynth() is the only route to an instance. The base is
 * not declared yet, because nothing in the application layer uses it.
 *
 * The title comes from the RTTI and refers to the hardware the class drives, not to a platform
 * split in this header. Any divergence belongs in the implementation file.
 *
 * This declaration exists to satisfy the reference from Globals, which creates the single instance
 * in CreateSynth() and destroys it in Shutdown().
 */
class Ps2HardSynth {
public:
    virtual ~Ps2HardSynth();
};

/**
 * Create the hardware synthesiser.
 *
 * @return The new synthesiser, billed to the allocation tag at `0x00814e90`.
 * @ghidraAddress 0x003f4db8
 */
Ps2HardSynth *CreatePs2HardSynth();

/**
 * Advance the streaming voice buffers.
 *
 * MainLoop::Poll() drives this once per frame.
 *
 * @ghidraAddress 0x00464bc8
 */
void PollSynthStream();

/**
 * Advance the synthesiser's pending events.
 *
 * One of MainLoop's two periodic timers drives this.
 *
 * @ghidraAddress 0x004648c8
 */
void PollSynthEvents();
