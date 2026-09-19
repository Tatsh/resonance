#pragma once

#include "met/metscreen.h"

/**
 * Logo panel drawn along the top of the front end.
 *
 * `16MetTopLogoScreen` in the RTTI descriptor at `0x008f0880`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x0080ff98`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x003c46f8` takes only the renderer and the load priority, and supplies `lp`
 * for the screen name, `metagame/Shared` for the directory, and `freq_logo_panel` for the
 * container. It writes nothing beyond its own vptr.
 *
 * The class declares no data member. It loads the same container as MetLogoScreen,
 * `metagame/Shared/freq_logo_panel`, under the screen name `lp` rather than `fl`, and it is one of
 * only three classes that inherit slot 5 unchanged.
 *
 * The object is at least 0x8c bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x003c7798`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 26 `0x003c77f0`, 38 `0x003c4868`.
 */
class MetTopLogoScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003c46f8
     */
    MetTopLogoScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003c7798
     */
    virtual ~MetTopLogoScreen();
};
