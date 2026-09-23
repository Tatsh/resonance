#pragma once

#include "app/animrange.h"
#include "app/tnlemitter.h"

/**
 * Particle burst that plays along "multfx.path" for one run.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the objects it drives, "multfx.path", "multfx.ps", and
 * "multfxa.ps".
 *
 * Start() arms a run of the path and restarts both systems. When the run ends, SetFrame() silences
 * them.
 *
 * AppTunnel allocates one, 0x60 bytes, and stores it at `+0x1c`. The destructor is the implicit
 * one, and the image has no out-of-line copy.
 */
class TnlMultFX {
public:
    /**
     * Resolve the path and both systems and silence the systems.
     *
     * @ghidraAddress 0x0043d008
     */
    TnlMultFX();

    /**
     * Arm a run of the path between two frames and restart both systems.
     *
     * AppTunnel inlines this, and the out-of-line copy has no caller.
     *
     * @param flFrom The first path frame.
     * @param flTo The last path frame.
     * @ghidraAddress 0x004564a0
     */
    void Start(float flFrom, float flTo);

    /**
     * Drive the first system to flFrame and advance the run.
     *
     * The first system must be loaded. Once the run is over, both systems are silenced. AppTunnel
     * inlines this at `0x00447100`, and the out-of-line copy has no caller.
     *
     * @param flFrame The current frame.
     * @ghidraAddress 0x00456528
     */
    void SetFrame(float flFrame);

private:
    AnimRange mRange;       // Run of "multfx.path".
    TnlEmitter mEmitter;    // "multfx.ps".
    TnlEmitter mAltEmitter; // "multfxa.ps".
    int mUnknown50;         // +0x50, never written or read by the recovered routines.
    int mActive;
};
