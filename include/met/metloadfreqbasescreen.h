#pragma once

#include "met/metbuttonlist.h"
#include "met/metscreen.h"

/**
 * Base of the three screens that pick a saved FreQ identity.
 *
 * `21MetLoadFreqBaseScreen` in the RTTI descriptor at `0x008f2a30`, with MetScreen as its one
 * public non-virtual base at offset 0. The object is 0xa4 bytes, and two children fix that
 * independently, MetLoadFreqScreen by placing MemcardUser at `+164` and MetLoadNewFreqScreen by
 * placing MetKBUser at `+164`.
 *
 * Three classes derive from the class, MetLoadFreqScreen, MetLoadNewFreqScreen, and
 * MetLoadPreFabScreen.
 *
 * The primary vtable at `0x007f6700` has 47 entries, eight more than the MetScreen table, so the
 * class declares eight virtuals of its own at slots 39 through 46, at `0x00292620`, `0x00296a50`,
 * `0x00293028`, `0x00293148`, `0x002933b8`, `0x00296d08`, `0x00292810`, and `0x00296c88`. That is
 * the widest table in the subsystem and the largest interface any Met class adds. None of the
 * eight has a recovered name, so all eight are recorded rather than declared.
 *
 * The constructor at `0x00291e00` takes only the renderer and the load priority, and supplies
 * `cid` for the screen name, `metagame/_Solo` for the directory, and `create_id` for the
 * container. All three children call it, so all three load the same container and differ only in
 * behaviour. It allocates a MetButtonList tagged `MetButtonList` into mUnknown90, zeroes
 * mUnknown94, and stores the result of `0x001712c0` into mUnknowna0. The words at `+0x98` and
 * `+0x9c` are never written.
 *
 * The destructor at `0x00296ae0` restores the vptr, runs the MetScreen destructor, and releases
 * the object with the tag `MsgSink`. It releases nothing of its own.
 *
 * Seven inherited slots differ from the MetScreen table, and all seven bodies are shared by all
 * three children, which is what proves they belong here. Slots 23 and 24 at `0x00296b60` and
 * `0x00296bb0` are the cycle sounds, and the rest are 5 `0x002926d8`, 19 `0x00292178`,
 * 30 `0x00292c60`, 36 `0x00292da8`, and 38 `0x00292000`.
 */
class MetLoadFreqBaseScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00291e00
     */
    MetLoadFreqBaseScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00296ae0
     */
    virtual ~MetLoadFreqBaseScreen();

    /**
     * @ghidraAddress 0x00296b60
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x00296bb0
     */
    virtual void PlayCycleRightSound();

private:
    MetButtonList *mUnknown90; // +0x90
    int mUnknown94;            // +0x94
    int mUnknown98;            // +0x98, not written by the constructor
    int mUnknown9c;            // +0x9c, not written by the constructor
    int mUnknowna0;            // +0xa0, from the routine at 0x001712c0
};
