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
 * Its RTTI descriptor is at `0x009021e0`. It has MetScreen as its one public non-virtual base at
 * offset 0. New() allocates 0x100 bytes, the highest member ending at `+0xf8` and the
 * quadword-aligned Vector3 members rounding the total up.
 *
 * The 39-entry primary vtable is at `0x00802420`, the same length as the MetScreen table, so the
 * class declares no virtual of its own. It is one of only three classes that inherit slot 5
 * unchanged rather than overriding it.
 *
 * A prompt is posted through SetText(). The screen fills the six info texts from script template
 * 0x268, or in the European release from the prompt text itself, and plays `so_TT_01.anim` to show
 * them. A new prompt arriving while one is shown plays `so_TT_02.anim` to hide the old one first,
 * and the new one is shown when the hide finishes.
 * UpdateIdle() advances both animations. The four title texts are filled the same way from
 * the layout SelectPreset() chooses.
 */
class MetHelpScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * The screen name is `so`, the directory `metagame/shared`, and the container `options_sl`.
     * MetScreen::mShowsLoadedDrawables is cleared.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x003125b0
     * @ghidraAddress PAL: 0x00338418
     */
    MetHelpScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x00312770
     * @ghidraAddress PAL: 0x00338640
     */
    virtual ~MetHelpScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x00317210
     * @ghidraAddress PAL: 0x0033d4c8
     */
    static MetHelpScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Replace the shared prompt text and repost it.
     *
     * The routine resolves the screen registered under the literal `MetHelpScreen`, casts it to
     * this class, and forwards to PostText(). A front end with no help screen registered forwards
     * through a null receiver. It is a static member rather than a free function, because it takes
     * no receiver and vends exactly one class. The North American release passes a key that
     * FillTexts() looks up, and the European release passes the text itself.
     *
     * @param text The prompt to display.
     * @param flTime The renderer time to post the prompt at.
     * @ghidraAddress NTSC-U/C: 0x00317368
     * @ghidraAddress PAL: 0x0033d550
     */
    static void SetText(const HxStr &text, float flTime);

    /**
     * Select one named prompt layout.
     *
     * The routine resolves the same registered screen SetText() does and forwards to
     * ApplyPreset(). MetLoadFreqScreen::EnterAndShow() passes `standard_title`. The European
     * release passes the layout text itself rather than a name.
     *
     * @param name The layout name.
     * @ghidraAddress NTSC-U/C: 0x00317298
     * @ghidraAddress PAL: 0x00338328
     */
    static void SelectPreset(const HxStr &name);

    /**
     * Record one named layout and fill the title texts from it.
     *
     * The titles are filled only once the container views are resolved.
     *
     * @param name The layout name.
     * @ghidraAddress NTSC-U/C: 0x00317758
     * @ghidraAddress PAL: 0x0033d960
     */
    void ApplyPreset(const HxStr &name);

    /**
     * Post one prompt at one time.
     *
     * An empty prompt over an empty one, the prompt already shown while no animation runs, and a
     * screen whose views are not resolved yet are all ignored. With nothing shown the prompt is
     * shown at once. Otherwise the hide animation starts and the prompt waits in mWaitingText.
     *
     * @param text The prompt to display.
     * @param flTime The renderer time to post the prompt at.
     * @ghidraAddress NTSC-U/C: 0x00312f70
     * @ghidraAddress PAL: 0x00338f20
     */
    void PostText(const HxStr &text, float flTime);

    /**
     * Depart on a back command.
     *
     * Slot 19.
     *
     * @param pCommand The command.
     * @ghidraAddress NTSC-U/C: 0x00317448
     * @ghidraAddress PAL: 0x0033d650
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Advance the show and hide animations.
     *
     * Slot 26.
     *
     * @param flTime The renderer time.
     * @ghidraAddress NTSC-U/C: 0x00312df0
     * @ghidraAddress PAL: 0x00338da0
     */
    virtual void UpdateIdle(float flTime);

    /**
     * Empty the info texts, forget both prompts, and stop the hide animation.
     *
     * Slot 36. The show animation's start time is not cleared.
     *
     * @ghidraAddress NTSC-U/C: 0x00317480
     * @ghidraAddress PAL: 0x0033d688
     */
    virtual void OnExitFinished();

    /**
     * Resolve the container views, the six info texts, the four title texts, and both animations.
     *
     * Slot 38. The texts are `sl_pan_opt_info_0N.txt` and `sl_opt_title_0N.txt` for N from 1, each
     * emptied and shown. The resting translation of the first text of each group is recorded. The
     * animations are `so_TT_01.anim` and `so_TT_02.anim`, whose end frames are recorded when they
     * resolve. No text is tested for null.
     *
     * @ghidraAddress NTSC-U/C: 0x003128d0
     * @ghidraAddress PAL: 0x003387d0
     */
    virtual void ResolveContainerViews();

private:
    /**
     * Empty every info text and recompose the root view's world transform.
     *
     * @ghidraAddress NTSC-U/C: 0x003130e8
     * @ghidraAddress PAL: 0x00339098
     */
    void ClearInfoTexts();

#ifdef VIDEO_STANDARD_PAL
    /**
     * Map a font code of the help text to a font name.
     *
     * `F1` is `font1_plain_3`, `F2` is `font1_blue_1`, and `FC` is `big_controller.font`. Any other
     * code is `font1_plain_3`. The name is inferred.
     *
     * @param code The code between the angle brackets, such as `F1`.
     * @return The font name.
     * @ghidraAddress PAL: 0x003391d8
     */
    static HxStr FontForCode(const HxStr &code);
#endif

    /**
     * Fill a group of texts from a list of runs, each a font name and a text.
     *
     * The North American release takes the list script template 0x268 returns for a key. Every
     * text of the group is emptied first, and each list entry that is a tuple gives the run for the
     * text at the same index. A result that is not a list does not change the texts.
     *
     * The European release splits the text itself into runs. A font code such as `<F1>` starts a
     * run, the text before the first code is in font code `F1`, and FontForCode() gives each font
     * name. A code with no closing bracket makes the split loop forever. With no runs, the texts
     * are unchanged. Otherwise every text of the group is emptied first.
     *
     * Each run is placed after the one before it, and the texts are then shifted left by half
     * their combined width so the run is centred on the origin. The texts are indexed without a
     * bounds test.
     *
     * @param key The prompt or layout to look up, or in the European release the text itself.
     * @param texts The texts to fill.
     * @param origin The resting translation of the first text.
     * @param nTitles Zero for the info texts, whose root view is then recomposed.
     * @ghidraAddress NTSC-U/C: 0x00313208
     * @ghidraAddress PAL: 0x00339300
     */
    void FillTexts(const HxStr &key,
                   std::vector<Rnd::Text *> &texts,
                   const Vector3 &origin,
                   int nTitles);

    /**
     * Show mShownText and start the show animation, unless it already runs.
     *
     * PostText() and UpdateHide() have the body expanded in place, and this copy has no caller.
     *
     * @param flTime The renderer time.
     * @ghidraAddress NTSC-U/C: 0x00317508
     * @ghidraAddress PAL: 0x0033d710
     */
    void StartShow(float flTime);

    /**
     * Advance the show animation, starting the hide when it finishes with a prompt waiting.
     *
     * UpdateIdle() has the body expanded in place, and this copy has no caller.
     *
     * @param flTime The renderer time.
     * @ghidraAddress NTSC-U/C: 0x00317568
     * @ghidraAddress PAL: 0x0033d770
     */
    void UpdateShow(float flTime);

    /**
     * Start the hide animation, unless it already runs.
     *
     * @param flTime The renderer time.
     * @ghidraAddress NTSC-U/C: 0x00317600
     * @ghidraAddress PAL: 0x0033d808
     */
    void StartHide(float flTime);

    /**
     * Advance the hide animation, then show the waiting prompt or empty the texts.
     *
     * UpdateIdle() has the body expanded in place, and this copy has no caller.
     *
     * @param flTime The renderer time.
     * @ghidraAddress NTSC-U/C: 0x00317640
     * @ghidraAddress PAL: 0x0033d848
     */
    void UpdateHide(float flTime);

    float mShowStart;                     // Time the show animation started, zero while idle.
    float mHideStart;                     // Time the hide animation started, zero while idle.
    HxStr mShownText;                     // The prompt shown.
    HxStr mWaitingText;                   // The prompt waiting for the hide to finish.
    std::vector<Rnd::Text *> mInfoTexts;  // The six info texts.
    Vector3 mInfoOrigin;                  // Resting translation of the first info text.
    std::vector<Rnd::Text *> mTitleTexts; // The four title texts.
    Vector3 mTitleOrigin;                 // Resting translation of the first title text.
    HxStr mPreset;                        // The layout ApplyPreset() last recorded.
    Rnd::Animatable *mShowAnim;           // `so_TT_01.anim`, the show animation.
    float mShowEnd;                       // End frame of the show animation.
    Rnd::Animatable *mHideAnim;           // `so_TT_02.anim`, the hide animation.
    float mHideEnd;                       // End frame of the hide animation.
};
