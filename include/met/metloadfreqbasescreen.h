#pragma once

#include <vector>

#include "met/metbuttonlist.h"
#include "met/metscreen.h"
#include "rnd/button.h"
#include "rnd/object.h"
#include "rnd/tex.h"

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
 * mUnknown94, waits for the FreQ maker assets, and resolves
 * `persona_texburn_texture_1.tex` into mBurnTexture. The words at `+0x98` and `+0x9c` are written
 * by slot 38 rather than by the constructor.
 *
 * The destructor at `0x00296ae0` restores the vptr, deletes mUnknown90 through slot 1 of the
 * MetButtonList table with the deleting `__in_chrg` value, runs the MetScreen destructor, and
 * releases the object with the tag `MsgSink`.
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
     * Play the cycle-left sound while the carousel is selected and has more than one entry.
     *
     * The override tests neither the selector nor any recorded selector of its own. It forwards to
     * MetScreen with the same selector when MetButtonList::mSelected is zero and the identity list
     * at mUnknown8c has at least kMinimumCyclableEntries entries.
     *
     * @param nSelector Passed through to MetScreen unchanged.
     * @ghidraAddress 0x00296b60
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play the cycle-right sound under the same two conditions as PlayCycleLeftSound().
     *
     * @param nSelector Passed through to MetScreen unchanged.
     * @ghidraAddress 0x00296bb0
     */
    virtual void PlayCycleRightSound(int nSelector);

private:
    // The identity list the carousel steps through. Both cycle sounds and slot 5 read it, and no
    // routine of this class writes it, so it is filled from outside the class. Only the four-byte
    // element width is recovered, and the element type follows the tree's default for a four-byte
    // scene-object vector. +0x8c
    std::vector<Rnd::Object *> *mUnknown8c;
    MetButtonList *mUnknown90; // +0x90
    // Index into the list at mUnknown8c. Slot 5 clears it once it has run past the end. +0x94
    int mUnknown94;
    // Slot 38 resolves both by name and casts each to Rnd::Button. +0x98 and +0x9c
    Rnd::Button *mUnknown98;
    Rnd::Button *mUnknown9c;
    // The texture `persona_texburn_texture_1.tex`, resolved by the constructor. +0xa0
    Rnd::Tex *mBurnTexture;
};
