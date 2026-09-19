#pragma once

#include <vector>

#include "met/metscreen.h"
#include "rnd/object.h"

/**
 * Panel that shows the stored data of one remix.
 *
 * `18MetRemixDataScreen` in the RTTI descriptor at `0x00901c00`, with MetScreen as its one public
 * non-virtual base at offset 0. The object is 0xb0 bytes and the 39-entry vtable is at
 * `0x00806a38`, the same length as the MetScreen table, so the class declares no virtual of its
 * own.
 *
 * The constructor at `0x00344740` takes only the renderer and the load priority, and supplies
 * `mcdat` for the screen name, `metagame/Shared` for the directory, and `memcard_remix_data` for
 * the container. It empties the three vectors below. The constructor resolves two texture pairs
 * into its own vectors, `gSongLogo1.tex` with `gSongLogo2.tex` and `gSongLabel1.tex` with
 * `gSongLabel2.tex`, through the helper at `0x00246de0`.
 *
 * The destructor at `0x00344bd0` releases the three vectors through the helper at `0x001fc568`,
 * returns their buffers to the pool, runs the MetScreen destructor, and releases the object with
 * the tag `MsgSink`.
 *
 * Five slots differ from the MetScreen table, and only the destructor has a recovered name. The
 * rest are 5 `0x00349a20`, 26 `0x00345de8`, 36 `0x00349ab8`, and 38 `0x00344d70`.
 */
class MetRemixDataScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00344740
     */
    MetRemixDataScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00344bd0
     */
    virtual ~MetRemixDataScreen();

private:
    std::vector<Rnd::Object *> mUnknown8c; // +0x8c
    std::vector<Rnd::Object *> mUnknown98; // +0x98
    std::vector<Rnd::Object *> mUnknowna4; // +0xa4
};
