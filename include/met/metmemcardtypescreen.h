#pragma once

#include "met/metbuttonlist.h"
#include "met/metscreen.h"
#include "os/hxstr.h"

/**
 * Screen that picks what kind of saved data to load.
 *
 * `20MetMemCardTypeScreen` in the RTTI descriptor at `0x00901f30`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007fc488`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x002d23b8` takes only the renderer and the load priority, and supplies
 * `mcrf` for the screen name, `metagame/Shared` for the directory, and `mcrf_load` for the
 * container. It zeroes mUnknown8c, sets mUnknown90, mUnknown9c, and mUnknowna0 to -1, zeroes
 * mUnknowna4, and constructs mUnknown94 from the empty literal at `0x007fc300`.
 *
 * It then pushes the object names `mcrf_remix` and `mcrf_freq` into the container object-name
 * vector MetScreen declares at `+0x38`.
 *
 * The object is at least 0xa8 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor at `0x002d84d0` deletes mUnknown8c through slot 1 of the MetButtonList table
 * with the deleting `__in_chrg` value, and the release of mUnknown94 that follows it is the
 * compiler-generated member teardown.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x002d2b50`, 19 `0x002d2898`, 23 `0x002d8438`, 24 `0x002d8440`, 30 `0x002d2cf0`, 36
 * `0x002d2e90`, 38 `0x002d26b0`.
 */
class MetMemCardTypeScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002d23b8
     */
    MetMemCardTypeScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002d84d0
     */
    virtual ~MetMemCardTypeScreen();

    /**
     * Silence the cycle-left sound.
     *
     * Both overrides are two-instruction stubs, so each was written inline with an empty body.
     *
     * @ghidraAddress 0x002d8438
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * @ghidraAddress 0x002d8440
     */
    virtual void PlayCycleRightSound(int) {
    }

private:
    // Zeroed by the constructor and deleted by the destructor. Its vptr sits at `+0x14` of the
    // object, which is where MetButtonList places one. +0x8c
    MetButtonList *mUnknown8c;
    int mUnknown90;   // +0x90, starts at -1
    HxStr mUnknown94; // +0x94, constructed from the empty literal
    int mUnknown9c;   // +0x9c, starts at -1
    int mUnknowna0;   // +0xa0, starts at -1
    int mUnknowna4;   // +0xa4
};
