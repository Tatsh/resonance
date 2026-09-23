#pragma once

#include <vector>

#include "math/color.h"
#include "met/metscreen.h"

namespace Rnd {
class Mat;
class Mesh;
class Text;
} // namespace Rnd

/**
 * End-of-game statistics for a multiplayer session.
 *
 * `19MetMultiStatsScreen` in the RTTI descriptor at `0x008eedd8`, with MetScreen as its one public
 * non-virtual base at offset 0. The object is 0xdc bytes, the size New() requests.
 *
 * The 39-entry primary vtable is at `0x008003c0`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The screen shows the song and difficulty, then up to four player rows ordered by descending
 * score. Each row has a score text, a name text, a persona picture material, and a mesh tinted
 * with the player's colour.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x00300268`, 36 `0x00306998`, and 38 `0x002ff3f8`.
 */
class MetMultiStatsScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * The screen name is `ems`, the directory `metagame/_Local`, and the container
     * `end_multi_stats`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002ff228
     */
    MetMultiStatsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002fff90
     */
    virtual ~MetMultiStatsScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00306910
     */
    static MetMultiStatsScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Fill the song, the difficulty, and the player rows, then enter.
     *
     * Slot 5. A loading game names the song from MetRemixManager's current record. Any other game
     * names it from configuration code 0x325, or code 0x327 when that is wider than the text
     * wraps at. The players are ordered by descending score through an insertion into
     * mUnknownc4. Outside a net game, the persona names and burn textures come from
     * MetFrontEndState::mUnknown00 in order, and each persona is attached to its burn slot. Scores
     * show only when GameParams::mUnknown1c is 1. Rows past the player count are emptied and
     * their meshes hidden.
     *
     * @ghidraAddress 0x00300268
     */
    virtual void EnterAndShow();

    /**
     * Slot 36, overridden empty.
     *
     * @ghidraAddress 0x00306998
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the container views, the headings, and the four player rows.
     *
     * Slot 38. The three headings take their text from configuration code 0x258. The four rows
     * append `ems_score_0N.txt`, `ems_freqname_0N.txt`, `ems_freq_0N.mat`, and
     * `ems_freq_0N.mesh` for N from 1, emptying both texts. The four player colours are green,
     * violet, yellow, and red. No object is tested for null.
     *
     * @ghidraAddress 0x002ff3f8
     */
    virtual void ResolveContainerViews();

private:
    Rnd::Text *mUnknown8c;               // +0x8c, the song name
    Rnd::Text *mUnknown90;               // +0x90, the difficulty
    std::vector<Rnd::Text *> mUnknown94; // +0x94, the score texts
    std::vector<Rnd::Text *> mUnknowna0; // +0xa0, the name texts
    std::vector<Rnd::Mat *> mUnknownac;  // +0xac, the persona picture materials
    std::vector<Rnd::Mesh *> mUnknownb8; // +0xb8, the tinted meshes
    std::vector<int> mUnknownc4;         // +0xc4, the player indices by descending score
    std::vector<Color> mUnknownd0;       // +0xd0, the player colours
};
