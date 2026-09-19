#pragma once

#include <vector>

#include "met/metmemcardpickeruser.h"
#include "met/metmemdetectscreen.h"
#include "rnd/object.h"

/**
 * Screen that picks a memory card to load from.
 *
 * `20MetMemCardLoadScreen` in the RTTI descriptor at `0x00901f20`, with two public non-virtual
 * bases at fixed offsets, MetMemDetectScreen at `+0x00` and MetMemCardPickerUser at `+160`. The
 * object is at least 0xf0 bytes.
 *
 * The class emits **two** vtables, the 44-entry primary at `0x007fbde8` and the 21-entry
 * MemcardUser table at `0x007fbd38` that adjusts `this` by `-140`. It emits none for
 * MetMemCardPickerUser, and its constructor writes vptrs at `+0x00` and `+0x8c` and nothing at
 * `+0xa0`, which is a further confirmation that MetMemCardPickerUser declares no virtual function.
 *
 * The primary table is the same length as the MetMemDetectScreen table, so this class declares
 * **no** virtual of its own. Fifteen inherited slots differ, which makes it the most heavily
 * overriding class in the subsystem without extending the interface at all.
 *
 * The constructor at `0x002cb8c8` takes only the renderer and the load priority. It runs the
 * MetMemDetectScreen constructor with `mcl` for the screen name, `metagame/Shared` for the
 * directory, and `memcard_load` for the container, and pushes `mcl_card` into the container
 * object-name vector that MetScreen owns.
 *
 * The destructor at `0x002cbcc8` restores both vptrs, returns buffers to the pool, runs the
 * MetMemDetectScreen destructor, and releases the object with the tag `MsgSink`.
 *
 * Of the fifteen differing slots only the destructor has a recovered name. Slots 22 through 24 at
 * `0x002d1d80`, `0x002d1eb0`, and `0x002d1ef8` are real bodies rather than stubs, and the rest are
 * 5 `0x002cc9c0`, 19 `0x002cc328`, 20 `0x002d1f40`, 30 `0x002ccfb8`, 33 `0x002d2018`,
 * 36 `0x002cd0e8`, 38 `0x002cbeb8`, 39 `0x002ce2c0`, 41 `0x002cd4e0`, and 42 `0x002cdce0`.
 */
class MetMemCardLoadScreen : public MetMemDetectScreen, public MetMemCardPickerUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002cb8c8
     */
    MetMemCardLoadScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002cbcc8
     */
    virtual ~MetMemCardLoadScreen();

    /**
     * @ghidraAddress 0x002d1f40
     */
    virtual void PlaySlideSound();

    /**
     * @ghidraAddress 0x002d1d80
     */
    virtual void PlayHighSound();

    /**
     * @ghidraAddress 0x002d1eb0
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x002d1ef8
     */
    virtual void PlayCycleRightSound();

private:
    int mUnknowna4; // +0xa4
    int mUnknowna8; // +0xa8
    // Never written by the constructor and not recovered.
    unsigned char mUnknownac[0xc];         // +0xac
    std::vector<Rnd::Object *> mUnknownb8; // +0xb8
    std::vector<Rnd::Object *> mUnknownc4; // +0xc4
    int mUnknownd0;                        // +0xd0
    int mUnknownd4;                        // +0xd4
    std::vector<Rnd::Object *> mUnknownd8; // +0xd8
    int mUnknowne4;                        // +0xe4
};
