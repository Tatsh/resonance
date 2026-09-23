#pragma once

namespace Rnd {
class MatAnim;
} // namespace Rnd

/**
 * Material animation that plays out the tunnel lattice once it is triggered.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the one object it drives, "lattice.mnm", and from the script name
 * "nolattice".
 *
 * Until Start() the animation rests at frame 0. From the start frame on, SetFrame() drives it to
 * the time since the start, capped at 8160 frames. Destruction returns it to frame 0.
 *
 * AppTunnel allocates one, 8 bytes, and stores it at `+0x20`.
 */
class TnlLattice {
public:
    /**
     * Resolve "lattice.mnm" and rest it at frame 0.
     *
     * @ghidraAddress 0x0043c598
     */
    TnlLattice();

    /**
     * Return the animation to frame 0.
     *
     * @ghidraAddress 0x00456368
     */
    ~TnlLattice();

    /**
     * Record the frame the animation starts from.
     *
     * AppTunnel's message handler inlines the store at `0x00449418`, and no out-of-line copy
     * exists. The title is inferred.
     *
     * @param flFrame The start frame.
     */
    void Start(float flFrame) {
        mStartFrame = flFrame;
    }

    /**
     * Drive the animation to the time since the start frame.
     *
     * Returns at once before the start frame. AppTunnel inlines this at `0x0044715c`.
     *
     * @param flFrame The current frame.
     * @ghidraAddress 0x004563c0
     */
    void SetFrame(float flFrame);

private:
    Rnd::MatAnim *mMatAnim; // "lattice.mnm".
    float mStartFrame;      // 1e9 until Start().
};
