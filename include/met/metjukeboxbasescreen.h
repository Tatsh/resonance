#pragma once

#include "met/listdataprovider.h"
#include "met/metscreen.h"

/**
 * Base of the three jukebox screens.
 *
 * `20MetJukeboxBaseScreen` in the RTTI descriptor at `0x00902330`, with two public non-virtual
 * bases at fixed offsets, MetScreen at `+0x00` and ListDataProvider at `+140`. The class therefore
 * starts its own members at `+144`, and the object is 0xc8 bytes. All three children write their
 * own first member at `+0xc8`, which is what fixes that size.
 *
 * The constructor is at `0x0021dcc0`. It takes the renderer, the load priority, and the three
 * names, and all three children call it rather than inlining it.
 *
 * Three classes derive from the class, MetJukeboxCustomRemixesScreen,
 * MetJukeboxEditPlaylistScreen, and MetJukeboxFactoryRemixesScreen.
 *
 * Two vtables belong to the class. The primary table at `0x007ec6d0` has 43 entries, four more
 * than the MetScreen table, so the class declares four virtuals of its own at slots 39 through 42.
 * Slot 39 stores the `__pure_virtual` handler at `0x005381a8`, which makes the original class
 * abstract. None of the four has a recovered name, so all four are recorded rather than declared,
 * and this declaration is consequently instantiable where the original was not. The secondary
 * table at `0x007ec6a8` is addressed by the ListDataProvider vptr, adjusts `this` by `-140` in
 * every entry, and supplies the two ListDataProvider pure virtuals at `0x0021ef30` and
 * `0x00224ae8`.
 *
 * The four object names the screen resolves are `met_jukebox_base_screen_help_tab`,
 * `met_jukebox_base_screen_ticker_tape`, `met_jukebox_base_screen_error_tab`, and
 * `met_jukebox_base_screen_error_ticker_tape`, at `0x007ec590` through `0x007ec608`.
 *
 * Fifteen slots of the primary table differ from the MetScreen table. Slots 20 through 24 are
 * two-instruction `jr ra` stubs, so a jukebox screen plays none of those five sounds. None of the
 * remaining nine overrides has a recovered name.
 *
 *  - 1 `0x0021dfc0` the destructor.
 *  - 5 `0x0021e908` replaces the show-and-animate routine at `0x003900a8`.
 *  - 7 `0x00224a90` replaces an empty MetScreen slot.
 *  - 17 `0x00224af0` SetShowing().
 *  - 19 `0x0021e268` replaces an empty MetScreen slot.
 *  - 20 `0x00224968` PlaySlideSound(), overridden empty.
 *  - 21 `0x00224970` PlayLeaveSound(), overridden empty.
 *  - 22 `0x00224978` PlayHighSound(), overridden empty.
 *  - 23 `0x00224980` PlayCycleLeftSound(), overridden empty.
 *  - 24 `0x00224988` PlayCycleRightSound(), overridden empty.
 *  - 26 `0x0021fc88` replaces an empty MetScreen slot.
 *  - 33 `0x00224990` replaces an empty MetScreen slot.
 *  - 36 `0x002249e0` replaces an empty MetScreen slot.
 *  - 38 `0x0021e0f0` replaces the view-resolving routine at `0x0038b1b0`.
 *  - 39 through 42 the four virtuals this class declares, at `0x005381a8` for the pure one, then
 *    `0x0021f3e8`, `0x0021fe98`, and `0x0021e3f8`.
 */
class MetJukeboxBaseScreen : public MetScreen, public ListDataProvider {
public:
    /**
     * @ghidraAddress 0x0021dfc0
     */
    virtual ~MetJukeboxBaseScreen();

    /**
     * @param nShowing Non-zero to draw the screen.
     * @ghidraAddress 0x00224af0
     */
    virtual void SetShowing(int nShowing);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00224968
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * @ghidraAddress 0x00224970
     */
    virtual void PlayLeaveSound();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00224978
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00224980
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00224988
     */
    virtual void PlayCycleRightSound(int nSelector);
};
