#pragma once

class HxStr;

namespace Rnd {
class Mat;
class Mesh;
} // namespace Rnd

/**
 * Analog stick prompt of the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the `analog_stick` objects it resolves. The head-up display panel
 * embeds one at `+0x104`.
 *
 * The prompt is one mesh that shows one of two materials, the in-and-out stick or the up-and-down
 * stick.
 */
class HudAnalogStick {
public:
    /**
     * Resolve the prompt's mesh and both materials, and hide the prompt.
     *
     * @ghidraAddress 0x00417840
     */
    HudAnalogStick();

    /**
     * Put the material for one stick motion on the prompt.
     *
     * `in_out` selects `HUD in_out_stick.mat` and `up_down` selects `HUD up_down_stick.mat`. Any
     * other text changes nothing. The out-of-line copy has no caller. The title is inferred.
     *
     * @param motion The motion.
     * @ghidraAddress 0x00429d60
     */
    void SetMotion(const HxStr &motion);

    /**
     * Show or hide the prompt.
     *
     * The constructor is the one caller. The title is inferred.
     *
     * @param nShowing Non-zero to show.
     * @ghidraAddress 0x00429dd8
     */
    void SetShowing(int nShowing);

private:
    Rnd::Mesh *mMesh;     // `HUD1 analog_stick.mesh`
    Rnd::Mat *mInOutMat;  // `HUD in_out_stick.mat`
    Rnd::Mat *mUpDownMat; // `HUD up_down_stick.mat`
};
