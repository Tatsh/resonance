#pragma once

#include "met/metpausebasescreen.h"

/**
 * Pause screen of a net remix.
 *
 * `24MetPauseMultiRemixScreen` in the RTTI descriptor at `0x008ef480`, with MetPauseBaseScreen as
 * its one public non-virtual base at offset 0. The 40-entry vtable at `0x00804600` is the same
 * length as the MetPauseBaseScreen table, so the class declares no virtual of its own. New()
 * allocates 0xb0 bytes, the size of the base, so the class adds no member.
 *
 * The destructor at `0x0032b450` is compiler-generated.
 */
class MetPauseMultiRemixScreen : public MetPauseBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * The screen name is `pngr`, the directory `metagame/Transition`, and the container
     * `pause_net`. The panel a dismissed confirmation reactivates is `MetPauseMultiRemixScreen`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00327d90
     */
    MetPauseMultiRemixScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x0032b3c8
     */
    static MetPauseMultiRemixScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Fill the paused heading and the option labels, then enter.
     *
     * Slot 5. The heading is `psr_paused.txt`, the solo remix screen's text, read from
     * configuration code 0x258 under `pause_remix`. The labels come from code 0x259 under
     * `pause_multi_remix`. Every label is copied to the option text at the same index before
     * MetPauseBaseScreen::EnterAndShow() copies them again.
     *
     * @ghidraAddress 0x00328080
     */
    virtual void EnterAndShow();

    /**
     * Pass a back or code 10 to MetPauseBaseScreen::HandleCommand() and ignore every other code.
     *
     * Slot 19.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x0032b4a8
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Resolve the container views and the two option texts `pngr_opt1.txt` and `pngr_opt2.txt`.
     *
     * Slot 38. MetScreen::ResolveContainerViews() runs first. No text is tested for null.
     *
     * @ghidraAddress 0x00327f30
     */
    virtual void ResolveContainerViews();
};
