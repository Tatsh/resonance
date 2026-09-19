#pragma once

#include <vector>

#include "met/metloadfreqbasescreen.h"
#include "rnd/object.h"

/**
 * Screen that picks one of the pre-built FreQ identities.
 *
 * `19MetLoadPreFabScreen` in the RTTI descriptor at `0x008ef4f0`, with MetLoadFreqBaseScreen as
 * its one public non-virtual base at offset 0. The object is 0xb0 bytes and the 47-entry vtable at
 * `0x007f8c28` is the same length as the MetLoadFreqBaseScreen table, so the class declares no
 * virtual of its own.
 *
 * The constructor at `0x002a8a58` takes only the renderer and the load priority, runs the
 * MetLoadFreqBaseScreen constructor at `0x00291e00`, writes its own vptr, and empties the one
 * vector below. The destructor at `0x002ad450` restores the vptr, returns the vector buffer to the
 * pool, runs the MetLoadFreqBaseScreen destructor, and releases the object with the tag `MsgSink`.
 *
 * Ten slots differ from the MetLoadFreqBaseScreen table, and only the destructor has a recovered
 * name. The others are 5 `0x002a8aa0`, 15 `0x002a9b38`, 39 `0x002a8fe0`, 40 `0x002a94f0`,
 * 41 `0x002a9330`, 43 `0x002a9710`, 44 `0x002a91b8`, and 45 `0x002a8b80`.
 */
class MetLoadPreFabScreen : public MetLoadFreqBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002a8a58
     */
    MetLoadPreFabScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002ad450
     */
    virtual ~MetLoadPreFabScreen();

private:
    std::vector<Rnd::Object *> mUnknowna4; // +0xa4
};
