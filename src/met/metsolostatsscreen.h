#pragma once

#include "met/metscreen.h"

namespace Rnd {
class Mat;
class Tex;
class Text;
} // namespace Rnd

/**
 * End-of-game statistics for a solo session.
 *
 * `18MetSoloStatsScreen` in the RTTI descriptor at `0x008f0080`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x0080e218`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x003af2e0` takes only the renderer and the load priority, and supplies
 * `egs` for the screen name, `metagame/_Solo` for the directory, and `end_game_stats` for the
 * container. It writes nothing beyond its own vptr, and ResolveContainerViews() fills the ten
 * members. The factory's 0xb4-byte allocation fixes the size.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 and 38, both declared below.
 */
class MetSoloStatsScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003af2e0
     */
    MetSoloStatsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003b5218
     */
    virtual ~MetSoloStatsScreen();

    /**
     * Allocate and construct the screen.
     *
     * The 0xb4-byte allocation is billed to the tag `MsgSink`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x003b5190
     */
    static MetSoloStatsScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Fill the statistics of the finished session and show the screen. Slot 5.
     *
     * The first persona's username goes to the name field and its burn texture to the second
     * stage of `freq.mat`. The song field takes the level's title from configuration code 0x325,
     * or the name of the current remix when a saved game is loading, falling back to the short
     * title from code 0x327 when the text exceeds the field's wrap width. The skill field takes
     * the difficulty name, and the score, completion, phrase, and hottest fields take the first
     * player's GameStats figures, the two ratios as whole percentages.
     *
     * @ghidraAddress 0x003b0378
     */
    virtual void EnterAndShow();

    /**
     * Resolve the container objects this screen drives. Slot 38.
     *
     * Runs the MetScreen slot 38 body first, labels the nine heading texts from configuration code
     * 0x258 without storing them, and resolves the ten members. No lookup is tested for null
     * before use.
     *
     * @ghidraAddress 0x003af450
     */
    virtual void ResolveContainerViews();

private:
    Rnd::Mat *mFreqMat;       // +0x8c "freq.mat"
    Rnd::Mat *mRankMat;       // +0x90 "egs_rank.mat"
    Rnd::Tex *mBurnTex;       // +0x94 The first persona burn texture.
    Rnd::Text *mFreqNameText; // +0x98 "egs_freqname.txt"
    Rnd::Text *mScoreText;    // +0x9c "egs_score_val.txt"
    Rnd::Text *mSongText;     // +0xa0 "egs_song_val.txt"
    Rnd::Text *mSkillText;    // +0xa4 "egs_skill_val.txt"
    Rnd::Text *mCompleteText; // +0xa8 "egs_complete_val.txt"
    Rnd::Text *mPhraseText;   // +0xac "egs_phrase_val.txt"
    Rnd::Text *mHotText;      // +0xb0 "egs_hot_val.txt"
};
