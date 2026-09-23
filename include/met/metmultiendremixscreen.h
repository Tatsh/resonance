#pragma once

#include <vector>

#include "met/metscreen.h"
#include "met/texturepairrecord.h"

namespace Rnd {
class Mat;
class Mesh;
class Text;
} // namespace Rnd

/**
 * End-of-remix screen for a multiplayer session.
 *
 * `22MetMultiEndRemixScreen` in the RTTI descriptor at `0x008f07e0`, with MetScreen as its one
 * public non-virtual base at offset 0. The object is 0x120 bytes, the size New() allocates, and the
 * 39-entry vtable is at `0x007fed80`, the same length as the MetScreen table, so the class declares
 * no virtual of its own.
 *
 * The constructor at `0x002f0918` takes only the renderer and the load priority, and supplies
 * `erm` for the screen name, `metagame/Shared` for the directory, and `end_multi_remix` for the
 * container. It empties the three vectors below and builds the two texture pairs from
 * `gSongLogo1.tex` with `gSongLogo2.tex` and `gSongLabel1.tex` with `gSongLabel2.tex`.
 *
 * The screen shows the song's genre, tempo, logo, and label for the session's level, and a row of
 * four player slots. Each slot with a persona shows its burn texture, its freq mesh, and its name.
 * Slot 26 flips both texture pairs every frame, and slot 36 is empty.
 *
 * The destructor at `0x002f1500` destroys the two texture pairs, releases the three vectors, runs
 * the MetScreen destructor, and releases the object with the tag `MsgSink`.
 */
class MetMultiEndRemixScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002f0918
     */
    MetMultiEndRemixScreen(MetRenderer *pRenderer, int nPriority);

    /** @ghidraAddress 0x002f1500 */
    virtual ~MetMultiEndRemixScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002f5540
     */
    static MetMultiEndRemixScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Load the level's logo and label, show its genre and tempo, fill the four player slots from
     * MetFrontEndState's personas, set the `multi_remix_over` title and the `remix_save_options`
     * layout, push the help screen, and enter the screen.
     *
     * Slot 5.
     *
     * @ghidraAddress 0x002f16a0
     */
    virtual void EnterAndShow();

    /**
     * Advance both texture pairs and show each current texture on its material's first stage.
     *
     * Slot 26. Both Advance() results are discarded.
     *
     * @param flTime Not read.
     * @ghidraAddress 0x002f55c8
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Empty in this class. Slot 36.
     *
     * @ghidraAddress 0x002f5650
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the container's views, label the remix panel, and find the tempo and genre texts,
     * the logo and photo materials, and the name text, player material, and freq mesh of each of
     * the four player slots. Every name text starts empty.
     *
     * Slot 38.
     *
     * @ghidraAddress 0x002f0da8
     */
    virtual void ResolveContainerViews();

private:
    // ResolveContainerViews() fills +0x8c through +0xb4, and the constructor writes none of the
    // four pointers.
    Rnd::Text *mBpmText;                  // +0x8c "erm_bpm.txt"
    Rnd::Text *mGenreText;                // +0x90 "erm_genre.txt"
    Rnd::Mat *mLogoMat;                   // +0x94 "erm_logo.mat"
    Rnd::Mat *mPhotoMat;                  // +0x98 "erm_photo.mat"
    std::vector<Rnd::Text *> mNameTexts;  // +0x9c "erm_name_0<n>.txt"
    std::vector<Rnd::Mat *> mPlayerMats;  // +0xa8 "erm_player<n>.mat"
    std::vector<Rnd::Mesh *> mFreqMeshes; // +0xb4 "erm_freq_0<n>.mesh"
    TexturePairRecord mLogoTextures;      // +0xc0
    TexturePairRecord mLabelTextures;     // +0xf0
};
