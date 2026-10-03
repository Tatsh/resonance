#pragma once

#include "met/metscreen.h"
#include "os/hxstr.h"

namespace Rnd {
class Text;
} // namespace Rnd

/**
 * Title bar shown above another screen.
 *
 * Its RTTI descriptor is at `0x008ef8d0`. It has MetScreen as its one public non-virtual base at
 * offset 0.
 *
 * The 39-entry primary vtable is at `0x0080bc40`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x00390e10` takes only the renderer and the load priority, and supplies
 * `fst` for the screen name, `metagame/shared` for the directory, and `screen_title` for the
 * container. It starts mTitle empty. The factory's 0x98-byte allocation fixes the size.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5, 9, 33, 36, and 38, all declared below.
 */
class MetScreenTitleScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x00390e10
     * @ghidraAddress PAL: 0x003c2908
     */
    MetScreenTitleScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x00393fa0
     * @ghidraAddress PAL: 0x003c5a18
     */
    virtual ~MetScreenTitleScreen();

    /**
     * Allocate and construct the screen.
     *
     * The 0x98-byte allocation is billed to the tag `MsgSink`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x00393d78
     * @ghidraAddress PAL: 0x003c5990
     */
    static MetScreenTitleScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Show the recorded title and then the screen. Slot 5.
     *
     * @ghidraAddress NTSC-U/C: 0x003940b8
     * @ghidraAddress PAL: 0x003c5b60
     */
    virtual void EnterAndShow();

    /**
     * Start the exit. Slot 9.
     *
     * The override runs MetScreen::BeginExit() and nothing else.
     *
     * @ghidraAddress NTSC-U/C: 0x00394108
     * @ghidraAddress PAL: 0x003c5bb0
     */
    virtual void BeginExit();

    /**
     * Do nothing. Slot 33.
     *
     * @ghidraAddress NTSC-U/C: 0x00394100
     * @ghidraAddress PAL: 0x003c5ba8
     */
    virtual void OnEnterFinished();

    /**
     * Do nothing. Slot 36.
     *
     * @ghidraAddress NTSC-U/C: 0x00394128
     * @ghidraAddress PAL: 0x003c5bd0
     */
    virtual void OnExitFinished();

    /**
     * Resolve the title text `fst_title.txt` into mTitleText. Slot 38.
     *
     * Runs the MetScreen slot 38 body first.
     *
     * @ghidraAddress NTSC-U/C: 0x00390f88
     * @ghidraAddress PAL: 0x003c2ae8
     */
    virtual void ResolveContainerViews();

    /**
     * Replace the shared screen title.
     *
     * The routine resolves the screen registered under the literal `MetScreenTitleScreen`, casts
     * it to this class, and forwards to ApplyTitle(). The result is not tested for null. It is a
     * static member rather than a free function, because it takes no receiver and vends exactly
     * one class.
     *
     * @param title The title to display.
     * @ghidraAddress NTSC-U/C: 0x00393e00
     * @ghidraAddress PAL: 0x003c2728
     */
    static void SetTitle(const HxStr &title);

    /**
     * Record one title and push this screen.
     *
     * The title is copied to mTitle and the screen registered as `MetScreenTitleScreen` is pushed,
     * which shows it through EnterAndShow(). SetTitle() is the one caller.
     *
     * @param title The title to display.
     * @ghidraAddress NTSC-U/C: 0x00394010
     * @ghidraAddress PAL: 0x003c5a98
     */
    void ApplyTitle(const HxStr &title);

    /**
     * Replace the shared screen title without showing the panel again.
     *
     * The same lookup and cast as SetTitle(), forwarding to ReplaceTitleText() instead.
     * MetRemixLoadScreen calls it when the button ring changes the catalogue. The name is
     * inferred.
     *
     * @param title The title to display.
     * @ghidraAddress NTSC-U/C: 0x00393ed0
     * @ghidraAddress PAL: 0x003c2818
     */
    static void ReplaceTitle(const HxStr &title);

    /**
     * Record one title and set it on the title text.
     *
     * The title is copied to mTitle and set on mTitleText. Unlike ApplyTitle(), the screen is not
     * pushed again. The name is inferred.
     *
     * @param title The title to display.
     * @ghidraAddress NTSC-U/C: 0x00394130
     * @ghidraAddress PAL: 0x003c5bd8
     */
    void ReplaceTitleText(const HxStr &title);

private:
    Rnd::Text *mTitleText; // +0x8c "fst_title.txt", resolved by ResolveContainerViews()
    HxStr mTitle;          // +0x90 The title shown, empty until one is set.
};
