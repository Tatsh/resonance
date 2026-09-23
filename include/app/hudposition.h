#pragma once

#include <cmath>
#include <vector>

class PlayMap;

namespace Rnd {
class Font;
class Mat;
class Mesh;
class MeshAnim;
class Text;
class View;
} // namespace Rnd

/**
 * Song position bar of the head-up display, one labelled block per section of the level.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the `pos` objects it resolves. The head-up display panel embeds one
 * at `+0x00`.
 *
 * The constructor clones one block mesh and one label per section of the play map into the
 * `<layout> pos.view` container, side by side, with the shared animation `HUD pos.msnm` sizing each
 * block before it is cloned. As the song moves from bar to bar, the blocks before the current
 * section take the past material, the current one takes the current material at a larger scale,
 * and the rest take the plain material. The repeat marker sits on the current section while the
 * play map reports that section as repeating.
 */
class HudPosition {
public:
    /**
     * Build one block and one label per section of a play map.
     *
     * @param pPlayMap The play map of the level, from Globals::GetPlayMap().
     * @ghidraAddress 0x00419f88
     */
    HudPosition(PlayMap *pPlayMap);

    /**
     * Delete every block and label and reset the container's origin.
     *
     * The head-up display panel's implicit destructor calls this one, and Overlay's destructor
     * inlines the panel's.
     *
     * @ghidraAddress 0x0041ac18
     */
    ~HudPosition();

    /**
     * Restyle the blocks for a new song position once the bar changes.
     *
     * The head-up display panel inlines the body, and no out-of-line copy exists.
     *
     * @param flFrame The song position, in MIDI ticks.
     */
    void SetFrame(float flFrame) {
        const int nBar = static_cast<int>(std::floor(flFrame / kTicksPerBar));
        if (nBar != mBar) {
            mBar = nBar;
            Update();
        }
    }

private:
    // One section of the level, with its block and its label.
    struct Section {
        Rnd::Mesh *mBlock;
        Rnd::Text *mLabel;
        // The first bar of the section and its length in bars.
        int mStartBar;
        int mBarCount;
        // The frame of `HUD pos.msnm` the block was sized at. The repeat marker takes it too.
        float mFrame;
    };

    // MIDI ticks in one bar.
    static constexpr float kTicksPerBar = 1920.0f;

public:
    /**
     * Style every section block for the current bar.
     *
     * Gives each block the material, the scale, and the label font for its place relative to the
     * section of mBar, and places the repeat marker on that section when the play map reports it
     * as repeating. Public because Overlay's AdvanceSectionToggleMsg handler at `0x0041f440`
     * calls it. The title is inferred.
     *
     * @ghidraAddress 0x0041ad88
     */
    void Update();

private:
    std::vector<Section> mSections;
    PlayMap *mPlayMap;
    Rnd::View *mView;       // `<layout> pos.view`
    Rnd::MeshAnim *mAnim;   // `HUD pos.msnm`
    Rnd::Mat *mCurrentMat;  // `pos_current.mat`
    Rnd::Mat *mNormalMat;   // `pos_norm.mat`
    Rnd::Mat *mPastMat;     // `pos_past.mat`
    Rnd::Font *mBlackFont;  // `HUD pos_black.font`
    Rnd::Font *mWhiteFont;  // `HUD pos_white.font`
    Rnd::View *mRepeatView; // `HUD pos_repeat.view`
    // The width of all the blocks together.
    float mWidth;
    // The index of the current section, or -1.
    int mCurrentSection;
    // The bar Update() last styled for. Starts at -99999.
    int mBar;
    int mUnknown3c; // +0x3c
};
