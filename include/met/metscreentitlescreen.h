#pragma once

#include "met/metscreen.h"

/**
 * Title bar shown above another screen.
 *
 * `20MetScreenTitleScreen` in the RTTI descriptor at `0x008ef8d0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x0080bc40`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x00390e10` takes only the renderer and the load priority, and supplies
 * `fst` for the screen name, `metagame/shared` for the directory, and `screen_title` for the
 * container. It writes nothing beyond its own vptr.
 *
 * The class declares no data member.
 *
 * The object is at least 0x8c bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x00393fa0`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x003940b8`, 9 `0x00394108`, 33 `0x00394100`, 36 `0x00394128`, 38 `0x00390f88`.
 */
class MetScreenTitleScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00390e10
     */
    MetScreenTitleScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00393fa0
     */
    virtual ~MetScreenTitleScreen();

    /**
     * Replace the shared screen title.
     *
     * The routine resolves the screen registered under the literal `MetScreenTitleScreen`, casts
     * it to this class, and forwards to ApplyTitle(). A front end with no title screen registered
     * forwards through a null receiver, which ApplyTitle() tolerates. It is a static member rather
     * than a free function, because it takes no receiver and vends exactly one class.
     *
     * @param title The title to display.
     * @ghidraAddress 0x00393e00
     */
    static void SetTitle(const HxStr &title);

    /**
     * Apply one title.
     *
     * The body is not written. SetTitle() is the one caller.
     *
     * @param title The title to display.
     * @ghidraAddress 0x00394010
     */
    void ApplyTitle(const HxStr &title);

    /**
     * Replace the shared screen title without showing the panel again.
     *
     * The same lookup and cast as SetTitle(), forwarding to ReplaceTitleText() instead.
     * MetRemixLoadScreen calls it when the button ring changes the catalogue. The body is not
     * written, and the name is inferred.
     *
     * @param title The title to display.
     * @ghidraAddress 0x00393ed0
     */
    static void ReplaceTitle(const HxStr &title);

    /**
     * Record one title and set it on the title text.
     *
     * The title is copied to `+0x90` and set on the Rnd::Text at `+0x8c` through its slot 6.
     * Unlike ApplyTitle(), the panel is not activated again. The two offsets lie past the 0x8c
     * bytes the constructor accounts for, so both are members this header does not declare yet.
     * The body is not written, and the name is inferred.
     *
     * @param title The title to display.
     * @ghidraAddress 0x00394130
     */
    void ReplaceTitleText(const HxStr &title);
};
