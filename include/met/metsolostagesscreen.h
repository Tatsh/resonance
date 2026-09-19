#pragma once

#include "met/metscreen.h"

/**
 * Screen that picks a solo stage.
 *
 * `19MetSoloStagesScreen` in the RTTI descriptor at `0x008f0070`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x0080d910`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x0039e308` takes only the renderer and the load priority, and supplies `ss`
 * for the screen name, `metagame/_Solo` for the directory, and `stage_sel` for the container. It
 * writes `+0x8c`, `+0x90`, two vectors at `+0xbc` and `+0xc8`, then `+0xd4`, `+0xd8`, and a run of
 * nine further vectors from `+0xf8` onward.
 *
 * It builds the same two texture-name pairs as the end-of-remix screens, `gSongLogo1.tex` with
 * `gSongLogo2.tex` and `gSongLabel1.tex` with `gSongLabel2.tex`, and pushes the object name
 * `levels`. The nine trailing vectors sit behind registers the disassembly does not
 * resolve to members, so the layout past `+0xf8` and the total size are not recovered.
 *
 * The total size is not recovered, and nothing derives from the class, so no base offset in any
 * descriptor pins it either.
 *
 * The destructor is at `0x0039ef80`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x003a5810`, 9 `0x003a6690`, 19 `0x003a4810`, 20 `0x003aed40`, 23 `0x003aedb0`, 24
 * `0x003aee00`, 26 `0x003a4df8`, 30 `0x003a54b8`, 33 `0x003aee50`, 36 `0x003a6738`, 38
 * `0x0039f720`.
 */
class MetSoloStagesScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0039e308
     */
    MetSoloStagesScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0039ef80
     */
    virtual ~MetSoloStagesScreen();
};
