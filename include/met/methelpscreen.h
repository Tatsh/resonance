#pragma once

#include "met/metscreen.h"

/**
 * Help and options screen.
 *
 * `13MetHelpScreen` in the RTTI descriptor at `0x009021e0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x00802420`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x003125b0` takes only the renderer and the load priority, and supplies `so`
 * for the screen name, `metagame/shared` for the directory, and `options_sl` for the container. It
 * writes the seven words from `+0x8c` to `+0xa0`, two vectors at `+0xa4` and `+0xc0`, and `+0xe0`
 * with `+0xe4`.
 *
 * It is one of only three classes that inherit slot 5 unchanged rather than overriding it.
 *
 * The object is at least 0xe8 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x00312770`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 19 `0x00317448`, 26 `0x00312df0`, 36 `0x00317480`, 38 `0x003128d0`.
 */
class MetHelpScreen : public MetScreen {
public:
    /**
     * Construct the screen.
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
     * Replace the shared prompt text and repost it.
     *
     * The routine resolves the screen registered under the literal `MetHelpScreen`, casts it to
     * this class, and forwards to PostText(). A front end with no help screen registered forwards
     * through a null receiver, which PostText() tolerates. It is a static member rather than a
     * free function, because it takes no receiver and vends exactly one class.
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
     * ApplyPreset(). MetLoadFreqScreen::EnterAndShow() is the one recovered caller and it passes
     * `standard_title`, which is the whole of the evidence for the verb.
     *
     * @param name The layout name.
     * @ghidraAddress 0x00317298
     */
    static void SelectPreset(const HxStr &name);

    /**
     * Apply one named prompt layout.
     *
     * The body is not written. SelectPreset() is the one caller.
     *
     * @param name The layout name.
     * @ghidraAddress 0x00317758
     */
    void ApplyPreset(const HxStr &name);

    /**
     * Post one prompt at one time.
     *
     * The body is not written. MetHelpScreen and MetLoadFreqBaseScreen both arrive at it only
     * through SetText().
     *
     * @param text The prompt to display.
     * @param flTime The renderer time to post the prompt at.
     * @ghidraAddress 0x00312f70
     */
    void PostText(const HxStr &text, float flTime);
};
