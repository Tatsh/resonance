#pragma once

#include <vector>

#include "memcard/memcarduser.h"
#include "met/metscreen.h"
#include "os/hxstr.h"

/**
 * Dialogue screen that writes the global settings to a memory card.
 *
 * `28MetGlobalSettingsSaverScreen` in the RTTI descriptor at `0x008eedc8`, with two public
 * non-virtual bases at fixed offsets, MetScreen at `+0x00` and MemcardUser at `+140`. The object
 * is 0x9c bytes. Its primary 39-entry vtable is at `0x007f4b08`, the same length as the MetScreen
 * table, so the class declares no virtual of its own, and the 21-entry MemcardUser table at
 * `0x007f4a58` adjusts `this` by `-140` in every entry.
 *
 * The constructor at `0x0027c4f8` takes only the renderer and the load priority, and supplies
 * `dlg` for the screen name, `metagame/Shared` for the directory, and `dialogue` for the
 * container. The directory uses a capital S in the image, unlike the lowercase
 * `metagame/shared` that the three configuration screens use, and it is reproduced verbatim.
 *
 * The destructor at `0x0027c680` releases the string vector, restores the MemcardUser vptr to
 * `0x007daf78`, runs the MetScreen destructor, and releases the object with the tag `MsgSink`.
 *
 * Eight slots differ from the MetScreen table. Slots 23 and 24 are two-instruction `jr ra` stubs,
 * so this screen plays neither cycle sound. Of the rest only the destructor has a recovered name.
 *
 *  - 1 `0x0027c680` the destructor.
 *  - 5 `0x00282088` replaces MetScreen::EnterAndShow at `0x003900a8`.
 *  - 9 `0x002820f8` replaces MetScreen::BeginExit at `0x00390100`.
 *  - 15 `0x0027dc70` replaces an empty MetScreen slot, the same slot MetConfigControllerScreen
 *    fills with an HxStr-taking handler.
 *  - 23 `0x00281fb0` PlayCycleLeftSound(), overridden empty.
 *  - 24 `0x00281fb8` PlayCycleRightSound(), overridden empty.
 *  - 38 `0x00282068` replaces MetScreen::ResolveContainerViews at `0x0038b1b0`.
 */
class MetGlobalSettingsSaverScreen : public MetScreen, public MemcardUser {
public:
    /**
     * Construct the settings-saver dialogue.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0027c4f8
     */
    MetGlobalSettingsSaverScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0027c680
     */
    virtual ~MetGlobalSettingsSaverScreen();

    /**
     * @ghidraAddress 0x00281fb0
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x00281fb8
     */
    virtual void PlayCycleRightSound();

private:
    std::vector<HxStr> mUnknown90; // +0x90
};
