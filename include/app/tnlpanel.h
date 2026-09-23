#pragma once

#include "math/color.h"

class AppTunnel;
class Player;
namespace Rnd {
class Mat;
class Mesh;
class MeshAnim;
} // namespace Rnd

/**
 * One tunnel section whose material and colour follow the lane owner, with a timed show or erase.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the materials and animations it uses, "tunnel mat<n>", "powerbar
 * mat<n>", "panel axe mat", "panel scratch mat", "panel vox mat", "panel show.msnm", and "panel
 * erase.msnm".
 *
 * A panel drives the ring section mesh of one lane and slice. Refresh() picks the material, and
 * Apply() shows or hides the mesh and colours the lane divider after the lane owner. From the start
 * frame, Update() plays the show or erase animation, or hands a lane with no owner to a
 * TnlPanelFX, and then applies the result.
 *
 * AppTunnel builds these in its bar handler and in its message handler at `0x00447ba8`. Both
 * inline the constructor. The bar handler allocates 0x40 bytes and queues the panel through
 * `0x00447268`. The message handler builds it on the stack and applies it at once.
 */
class TnlPanel {
public:
    /** Kind of panel. It selects the material Refresh() applies. */
    enum Kind {
        kKindLane = 0,    /*!< A lane section, "tunnel mat<n>" or "powerbar mat<n>". */
        kKindAxe = 1,     /*!< "panel axe mat". */
        kKindScratch = 2, /*!< "panel scratch mat". */
        kKindVox = 3      /*!< "panel vox mat". */
    };

    /**
     * Record the section and pick its material.
     *
     * Every caller inlines the constructor, and this copy has no caller.
     *
     * @param nRing The lane.
     * @param nSlice The slice.
     * @param pPlayer The lane owner, or the null player.
     * @param nPowerbar -1 for a plain lane, selecting "tunnel mat<n>" over "powerbar mat<n>".
     * @param kind The kind of panel.
     * @param nShowing Non-zero when the section should show.
     * @param nColorIndex The material number, taken modulo 4.
     * @ghidraAddress 0x00457228
     */
    TnlPanel(int nRing,
             int nSlice,
             Player *pPlayer,
             int nPowerbar,
             Kind kind,
             int nShowing,
             int nColorIndex);

    /**
     * Set the frame Update() starts from.
     *
     * AppTunnel inlines the store at `0x00447314`, and no out-of-line copy exists.
     *
     * @param flFrame The start frame.
     */
    void SetStartFrame(float flFrame) {
        mStartFrame = flFrame;
    }

    /**
     * Pick the material and colour for the kind of panel.
     *
     * A lane uses "tunnel mat<n>" or "powerbar mat<n>" in white. The other kinds use their panel
     * material in white at half alpha.
     *
     * @ghidraAddress 0x004417f8
     */
    void Refresh();

    /**
     * Show or hide the section and colour the lane divider after the lane owner.
     *
     * In game mode a lane panel whose owner is a real player hides. A shown section takes the
     * material and colour.
     *
     * @ghidraAddress 0x00441c68
     */
    void Apply();

    /**
     * Run the panel from its start frame.
     *
     * The first call at or after the start frame picks the animation. A lane with a real owner
     * plays "panel erase.msnm" over 480 frames. A lane with no owner runs for 200 frames with no
     * animation, and in game mode it starts a TnlPanelFX through pTunnel. Any other kind applies at
     * once and, when it should show and was hidden, plays "panel show.msnm" over 480 frames.
     *
     * @param flFrame The current frame.
     * @param pTunnel The tunnel whose panel effects a lane with no owner starts.
     * @return 1 while the panel runs, and 0 once it has been applied.
     * @ghidraAddress 0x00441d98
     */
    int Update(float flFrame, AppTunnel *pTunnel);

private:
    int mRing;
    int mSlice;
    Rnd::Mesh *mSection; // The ring section of mRing and mSlice.
    Player *mPlayer;
    int mPowerbar;
    Kind mKind;
    int mShowing;
    int mColorIndex;
    Color mColor;
    Rnd::Mat *mMat;
    float mStartFrame; // 1e9 until SetStartFrame().
    float mDuration;   // Zero until Update() picks the animation.
    Rnd::MeshAnim *mAnim;
};
