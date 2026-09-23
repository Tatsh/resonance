#pragma once

class HxStr;

namespace Rnd {
class Mesh;
class Text;
} // namespace Rnd

/**
 * Track name label of one player's track display on the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the `track` objects it resolves. HudTrack embeds one at `+0x50`.
 */
class HudTrackLabel {
public:
    /**
     * Resolve the label's mesh and text, and show the mesh.
     *
     * @param nIndex The track display number that fills `<layout> track<n>.mesh` and `.txt`.
     * @ghidraAddress 0x00417d58
     */
    HudTrackLabel(int nIndex);

    /**
     * Set the label's text.
     *
     * The out-of-line copy has no caller. The title is inferred.
     *
     * @param text The text to show.
     * @ghidraAddress 0x00429e68
     */
    void SetText(const HxStr &text);

private:
    Rnd::Mesh *mMesh; // `<layout> track<n>.mesh`
    Rnd::Text *mText; // `<layout> track<n>.txt`
};
