#pragma once

#include <vector>

#include "app/linearramp.h"
#include "os/hxstr.h"

namespace Rnd {
class Mesh;
class View;
} // namespace Rnd

/**
 * Instrument pointer of one player's activator, with its dip and spin.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from its views, "ptr_<c>" and "ptr_<c>_spin.view", where `<c>` is the
 * first letter of the player's colour name.
 *
 * Three mesh pairs, one per instrument kind, sit under the pointer, and SetKind() shows at most
 * one. Spin() drops the spin view from a height of 0.1 to 0 at once and runs the pointer
 * animation. After 300 time units Update() stops the spin, and the view rises back over 60.
 *
 * TnlActivator embeds one, 0x44 bytes, at `+0x18`.
 */
class TnlPointer {
public:
    /**
     * Icon and base mesh of one instrument kind, 8 bytes.
     *
     * The structure emits no RTTI. The name is inferred.
     */
    struct MeshPair {
        /**
         * Resolve both meshes.
         *
         * The program lists no caller. The constructor inlines the body.
         *
         * @param iconName The icon mesh.
         * @param baseName The base mesh.
         * @ghidraAddress 0x00455508
         */
        void Init(const HxStr &iconName, const HxStr &baseName);

        /**
         * Show or hide both meshes.
         *
         * The program lists no caller. SetKind() inlines the body.
         *
         * @param nShowing Non-zero to show.
         * @ghidraAddress 0x004555e0
         */
        void SetShowing(int nShowing);

        /**
         * Set the alpha of the icon mesh's material.
         *
         * The program lists no caller. SetAlpha() inlines the body.
         *
         * @param flAlpha The alpha.
         * @ghidraAddress 0x00455640
         */
        void SetAlpha(float flAlpha);

        Rnd::Mesh *mIcon; /*!< "ptr_<c>_axe.mesh", "ptr_<c>_scratch.mesh", or "ptr_<c>_vox.mesh". */
        Rnd::Mesh *mBase; /*!< "ptr_<c>_plate.mesh", or "ptr_<c>_tgt.mesh" for the voice pair. */
    };

    /**
     * Resolve the views and the three mesh pairs, and stop the spin.
     *
     * @param colorName The player's colour name, whose first letter selects every object.
     * @ghidraAddress 0x0043a810
     */
    explicit TnlPointer(const HxStr &colorName);

    /**
     * Show the mesh pair of one instrument kind and stop the spin.
     *
     * Kind 1 shows the axe pair, 3 the scratch pair, and 4 the voice pair. Every other kind shows
     * no pair.
     *
     * @param nKind The instrument kind.
     * @ghidraAddress 0x0043b248
     */
    void SetKind(int nKind);

    /**
     * Stop the spin and send the dip ramp back to rest.
     *
     * @ghidraAddress 0x004557d0
     */
    void Reset();

    /**
     * Drop the pointer at once and spin it.
     *
     * Does nothing while the pointer view is hidden. The program lists no caller.
     *
     * @param nRestart Non-zero to restart the 300-unit spin period at the next Update().
     * @ghidraAddress 0x00455768
     */
    void Spin(int nRestart);

    /**
     * Set the sideways offset of the spin view from a lane position.
     *
     * The offset is `(flLane - 0.5) * -0.5`, computed in double precision. The program lists no
     * caller.
     *
     * @param flLane The lane position, 0 through 1.
     * @ghidraAddress 0x00455810
     */
    void SetLane(float flLane);

    /**
     * Set the alpha of every icon mesh's material.
     *
     * The program lists no caller. TnlActivator::Update() inlines the body.
     *
     * @param flAlpha The alpha.
     * @ghidraAddress 0x004556c8
     */
    void SetAlpha(float flAlpha);

    /**
     * Parent the pointer view to another view for both transform and drawing.
     *
     * The program lists no caller. TnlActivator's constructor inlines the body.
     *
     * @param pParent The parent view.
     * @ghidraAddress 0x00455670
     */
    void AttachTo(Rnd::View *pParent);

    /**
     * Advance the dip ramp and the spin.
     *
     * The program lists no caller. TnlActivator::Update() inlines the body.
     *
     * @param flTime The scaled song position TnlActivator::Update() receives.
     * @ghidraAddress 0x00455860
     */
    void Update(float flTime);

private:
    std::vector<MeshPair> mPairs;
    Rnd::View *mView;     // "ptr_<c>", whose animation is the spin.
    Rnd::View *mSpinView; // "ptr_<c>_spin.view", whose translation is the dip.
    float mLastTime;
    float mSpinFrame;
    int mSpinning;
    float mOffsetX;
    LinearRamp mDip;
    float mUnknown3c; // +0x3c, set to 0.5 and never read by the recovered routines.
    float mSpinStart; // 1e9 while no spin runs, -1e9 while a restart is pending.
};
