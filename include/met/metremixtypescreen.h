#pragma once

#include "met/metbuttonlist.h"
#include "met/metscreen.h"

/**
 * Screen that chooses what kind of remix to work on.
 *
 * `18MetRemixTypeScreen` in the RTTI descriptor at `0x00901c20`, with MetScreen as its one public
 * non-virtual base at offset 0. The object is 0x90 bytes and the 39-entry vtable is at
 * `0x00808760`, the same length as the MetScreen table, so the class declares no virtual of its
 * own.
 *
 * The constructor at `0x00361d18` takes only the renderer and the load priority, and supplies
 * `smrt` for the screen name, `metagame/Shared` for the directory, and `sm_remixtype` for the
 * container. It pushes three object names into the container object-name vector that MetScreen
 * owns, `smrt_new`, `smrt_load`, and `smrt_jukebox`, and then allocates a MetButtonList tagged
 * `MetButtonList` into the one member below.
 *
 * The destructor at `0x003696c0` restores the vptr, runs the MetScreen destructor, and releases
 * the object with the tag `MsgSink`. It releases nothing of its own, so the button list outlives
 * the screen.
 *
 * Nine slots differ from the MetScreen table. Slots 23 and 24 sit eight bytes apart at
 * `0x00369628` and `0x00369630` and are two-instruction `jr ra` stubs. Of the rest only the
 * destructor has a recovered name, and the others are 5 `0x00362600`, 15 `0x00364478`,
 * 19 `0x00362348`, 30 `0x00362fa0`, 36 `0x00363920`, and 38 `0x00362098`.
 */
class MetRemixTypeScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00361d18
     */
    MetRemixTypeScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003696c0
     */
    virtual ~MetRemixTypeScreen();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00369628
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00369630
     */
    virtual void PlayCycleRightSound(int nSelector);

private:
    MetButtonList *mUnknown8c; // +0x8c
};
