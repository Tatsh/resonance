#pragma once

#include <vector>

#include "math/vector3.h"
#include "met/metscreen.h"

namespace Rnd {
class Animatable;
class Text;
} // namespace Rnd

/**
 * Help and options screen, which also shows the prompt line every other screen posts.
 *
 * `13MetHelpScreen` in the RTTI descriptor at `0x009021e0`, with MetScreen as its one public
 * non-virtual base at offset 0. New() allocates 0x100 bytes, the highest member ending at `+0xf8`
 * and the quadword-aligned Vector3 members rounding the total up.
 *
 * The 39-entry primary vtable is at `0x00802420`, the same length as the MetScreen table, so the
 * class declares no virtual of its own. It is one of only three classes that inherit slot 5
 * unchanged rather than overriding it.
 *
 * A prompt is posted through SetText(). The screen fills the six info texts from script template
 * 0x268 and plays `so_TT_01.anim` to show them. A new prompt arriving while one is shown plays
 * `so_TT_02.anim` to hide the old one first, and the new one is shown when the hide finishes.
 * OnUnknownSlot26() advances both animations. The four title texts are filled the same way from
 * the layout SelectPreset() chooses.
 */
class MetHelpScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * The screen name is `so`, the directory `metagame/shared`, and the container `options_sl`.
     * MetScreen::mUnknown60 is cleared.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003125b0
     */
    MetHelpScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00312770
     */
    virtual ~MetHelpScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00317210
     */
    static MetHelpScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Replace the shared prompt text and repost it.
     *
     * The routine resolves the screen registered under the literal `MetHelpScreen`, casts it to
     * this class, and forwards to PostText(). A front end with no help screen registered forwards
     * through a null receiver. It is a static member rather than a free function, because it takes
     * no receiver and vends exactly one class.
     *
     * @param text The prompt to display.
     * @param flTime The renderer time to post the prompt at.
     * @ghidraAddress 0x00317368
     */
    static void SetText(const HxStr &text, float flTime);

    /**
     * Select one named prompt layout.
     *
     * The routine resolves the same registered screen SetText() does and forwards to
     * ApplyPreset(). MetLoadFreqScreen::EnterAndShow() passes `standard_title`.
     *
     * @param name The layout name.
     * @ghidraAddress 0x00317298
     */
    static void SelectPreset(const HxStr &name);

    /**
     * Record one named layout and fill the title texts from it.
     *
     * The titles are filled only once the container views are resolved.
     *
     * @param name The layout name.
     * @ghidraAddress 0x00317758
     */
    void ApplyPreset(const HxStr &name);

    /**
     * Post one prompt at one time.
     *
     * An empty prompt over an empty one, the prompt already shown while no animation runs, and a
     * screen whose views are not resolved yet are all ignored. With nothing shown the prompt is
     * shown at once. Otherwise the hide animation starts and the prompt waits in mUnknown9c.
     *
     * @param text The prompt to display.
     * @param flTime The renderer time to post the prompt at.
     * @ghidraAddress 0x00312f70
     */
    void PostText(const HxStr &text, float flTime);

    /**
     * Depart on a back command.
     *
     * Slot 19.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x00317448
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Advance the show and hide animations.
     *
     * Slot 26.
     *
     * @param flTime The renderer time.
     * @ghidraAddress 0x00312df0
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Empty the info texts, forget both prompts, and stop the hide animation.
     *
     * Slot 36. The show animation's start time is not cleared.
     *
     * @ghidraAddress 0x00317480
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the container views, the six info texts, the four title texts, and both animations.
     *
     * Slot 38. The texts are `sl_pan_opt_info_0N.txt` and `sl_opt_title_0N.txt` for N from 1, each
     * emptied and shown. The resting translation of the first text of each group is recorded. The
     * animations are `so_TT_01.anim` and `so_TT_02.anim`, whose end frames are recorded when they
     * resolve. No text is tested for null.
     *
     * @ghidraAddress 0x003128d0
     */
    virtual void ResolveContainerViews();

private:
    /**
     * Empty every info text and recompose the root view's world transform.
     *
     * @ghidraAddress 0x003130e8
     */
    void ClearInfoTexts();

    /**
     * Fill a group of texts from the list script template 0x268 returns for a key.
     *
     * Every text of the group is emptied first. Each list entry that is a tuple gives a font name
     * and a text for the text at the same index, and the texts are then shifted left by half their
     * combined width so the run is centred on the origin. A result that is not a list changes
     * nothing. The texts are indexed without a bounds test.
     *
     * @param key The prompt or layout to look up.
     * @param texts The texts to fill.
     * @param origin The resting translation of the first text.
     * @param nTitles Zero for the info texts, whose root view is then recomposed.
     * @ghidraAddress 0x00313208
     */
    void FillTexts(const HxStr &key,
                   std::vector<Rnd::Text *> &texts,
                   const Vector3 &origin,
                   int nTitles);

    /**
     * Show mUnknown94 and start the show animation, unless it already runs.
     *
     * PostText() and UpdateHide() have the body expanded in place, and this copy has no caller.
     *
     * @param flTime The renderer time.
     * @ghidraAddress 0x00317508
     */
    void StartShow(float flTime);

    /**
     * Advance the show animation, starting the hide when it finishes with a prompt waiting.
     *
     * OnUnknownSlot26() has the body expanded in place, and this copy has no caller.
     *
     * @param flTime The renderer time.
     * @ghidraAddress 0x00317568
     */
    void UpdateShow(float flTime);

    /**
     * Start the hide animation, unless it already runs.
     *
     * @param flTime The renderer time.
     * @ghidraAddress 0x00317600
     */
    void StartHide(float flTime);

    /**
     * Advance the hide animation, then show the waiting prompt or empty the texts.
     *
     * OnUnknownSlot26() has the body expanded in place, and this copy has no caller.
     *
     * @param flTime The renderer time.
     * @ghidraAddress 0x00317640
     */
    void UpdateHide(float flTime);

    float mUnknown8c;                    // Time the show animation started, zero while idle.
    float mUnknown90;                    // Time the hide animation started, zero while idle.
    HxStr mUnknown94;                    // The prompt shown.
    HxStr mUnknown9c;                    // The prompt waiting for the hide to finish.
    std::vector<Rnd::Text *> mUnknowna4; // The six info texts.
    Vector3 mUnknownb0;                  // Resting translation of the first info text.
    std::vector<Rnd::Text *> mUnknownc0; // The four title texts.
    Vector3 mUnknownd0;                  // Resting translation of the first title text.
    HxStr mUnknowne0;                    // The layout ApplyPreset() last recorded.
    Rnd::Animatable *mUnknowne8;         // `so_TT_01.anim`, the show animation.
    float mUnknownec;                    // End frame of the show animation.
    Rnd::Animatable *mUnknownf0;         // `so_TT_02.anim`, the hide animation.
    float mUnknownf4;                    // End frame of the hide animation.
};
