#pragma once

#include <vector>

#include "met/metscreen.h"
#include "rnd/object.h"

class MetButtonList;
class TexturePairRecord;

/**
 * Screen that picks an arena.
 *
 * `15MetArenasScreen` in the RTTI descriptor at `0x00901e80`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007e8ba0`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x001f65a0` takes only the renderer and the load priority, and supplies `as`
 * for the screen name, `metagame/_Solo` for the directory, and `arena_sel` for the container. It
 * pushes the object name `arenas` into the container object-name vector that MetScreen owns, and
 * allocates a MetButtonList into `+0x8c`.
 *
 * The object is 0xd4 bytes, which the allocation at `0x001fc6ac` fixes rather than the
 * constructor's highest store.
 *
 * The destructor is at `0x001f70f0`.
 *
 * Eleven entries of the primary table differ from the MetScreen table, which a diff of the two
 * settles rather than the title each routine carries. Apart from the type function they are 1
 * `0x001f70f0` the destructor, 5 `0x001f7750`, 19 `0x001f7310`, 20 `0x001fc718`, 23 `0x001fc680`,
 * 24 `0x001fc688`, 26 `0x001f8af0`, 30 `0x001f8378`, 33 `0x001fc758`, 36 `0x001f8658`, and 38
 * `0x001f6a08`. All ten behaviour slots are declared below.
 *
 * Slots 23 and 24 are two-instruction empty bodies at distinct addresses, and each is a genuine
 * override rather than an inherited empty body, because MetScreen's own slot 23 and 24 both play
 * a named sound. The class therefore silences both cycle sounds while overriding the slide sound
 * with a conditional one.
 *
 * Three private routines in the translation unit are reached only from the slots below and none
 * has a recovered name. They are `0x001f75c8`, which slot 19 runs after either navigation command,
 * `0x001f7d40`, which slot 5 runs last before selecting the prompt layout, and the arena table
 * getter at `0x003d06f8`, which takes no argument and belongs to the game layer rather than to
 * this class. One further routine, `0x001fc690`, allocates 0xd4 bytes under the tag `MsgSink`,
 * runs the constructor, and returns the object, which is what a `new` expression compiles to.
 *
 * Four bodies are written. Slots 20, 23, 24, and 33 need nothing this tree does not declare.
 * Slots 5, 19, 26, 30, 36, and 38 are understood and not written, and each needs a routine no
 * header declares yet: the two TexturePairRecord methods at `0x00249850` and `0x00249808`, the
 * material setter at `0x004dd0a0`, the three GameManagerImpl slots at table offsets 0xc0, 0xc8,
 * and 0xd8, the arena table getter at `0x003d06f8`, the front-end state getter at `0x00217f30`,
 * the two container-object collectors at `0x001fc008` and `0x001fc230`, and the two unnamed
 * private routines above.
 */
class MetArenasScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x001f65a0
     */
    MetArenasScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x001f70f0
     */
    virtual ~MetArenasScreen();

    /**
     * Title the screen from the session's mode and stage, and show it. Slot 5.
     *
     * The body is not written. It takes the GameParams the game manager reports through its table
     * offset 0xc8, copies them, reads the mode through offset 0xc0, and builds the title from the
     * configuration keys `solo` or `game` and `arenas`, concatenating the pieces with
     * HxStr::AppendChar() and HxStr::AppendOther() before handing the result to
     * MetScreenTitleScreen::SetTitle(). It then invalidates the texture pair, selects the prompt
     * layout `standard_title`, and calls MetScreen::EnterAndShow() directly.
     *
     * @ghidraAddress 0x001f7750
     */
    virtual void EnterAndShow();

    /**
     * Route one command to the button ring, the selection, or the departure. Slot 19.
     *
     * The body is not written, because the helper at `0x001f75c8` that both navigation commands
     * run is not identified.
     *
     * Commands 1 and 2 step the ring and then run that helper. Command 5 is accepted only while
     * the selected index is below mUnknown98 or equal to mUnknown94, and it then clears the prompt,
     * activates the empty panel name, and starts the selected button alternating with an interval
     * of 30.0f and two cycles. Command 6 clears the prompt, clears MetScreen::mUnknown18, exits
     * `MetScreenTitleScreen`, and starts the exit animation.
     *
     * @param pCommand The command to route.
     * @ghidraAddress 0x001f7310
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the slide sound only for a real arena. Slot 20.
     *
     * The base sound plays while the selected index is below mUnknown98 or equal to mUnknown94, and
     * no sound plays for any other index. The base implementation is called directly rather than
     * dispatched.
     *
     * @param nSelector The controller index.
     * @ghidraAddress 0x001fc718
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play no left cycle sound. Slot 23.
     *
     * @param nSelector The controller index, which the body does not read.
     * @ghidraAddress 0x001fc680
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play no right cycle sound. Slot 24.
     *
     * @param nSelector The controller index, which the body does not read.
     * @ghidraAddress 0x001fc688
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Advance the arena screenshot pair and hand the current texture to the panel mesh. Slot 26.
     *
     * The body is not written. The pair at mUnknown90 is advanced through the TexturePairRecord
     * method at `0x00249850` and read through the one at `0x00249808`. The mesh
     * `as_arena_panel.mesh` is resolved through Rnd::Manager::Find(), narrowed with a
     * `dynamic_cast` to Rnd::Mesh, and hidden with Rnd::Drawable::SetShowing(0) before the read.
     * A read that reports a texture shows the mesh again and hands the texture to the material
     * setter at `0x004dd0a0`, together with the field at `+0x1c` of the object at mUnknown9c.
     * A read that reports nothing returns with the mesh hidden, which is how the screen looks
     * while the screenshot is still loading.
     *
     * @param flTime The current renderer frame position, which the body does not read.
     * @ghidraAddress 0x001f8af0
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Commit the chosen arena and depart, or activate the panel for a non-arena entry. Slot 30.
     *
     * The body is not written. MetScreen slot 29 runs the slot once the selection has finished
     * alternating.
     *
     * A selected index below mUnknown98, or equal to mUnknown94, copies the game manager's
     * GameParams, assigns the name at index `selected * 12` of the arena table the getter at
     * `0x003d06f8` returns into the copy's HxStr at `+0x08`, hands the copy back through the game
     * manager's table offset 0xd8, sets MetScreen::mUnknown18 to 2, exits `MetHelpScreen` and
     * `MetScreenTitleScreen`, and starts the exit animation. Any other index activates the panel
     * named `MetArenasScreen` and does nothing else.
     *
     * @param pObject The object slot 29 finished with, which the body does not read.
     * @ghidraAddress 0x001f8378
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Post the first prompt. Slot 33.
     *
     * MetScreen slot 32 runs the slot once the enter animation has finished. The prompt is
     * MetScreen::mUnknown38's first element rather than the selected one, which is faithful.
     *
     * @ghidraAddress 0x001fc758
     */
    virtual void OnUnknownSlot33();

    /**
     * Go on to whichever screen the departure was for. Slot 36.
     *
     * The body is not written. MetScreen slot 35 runs the slot once the exit animation has
     * finished. It reads the screen name stored in the front-end state object the getter at
     * `0x00217f30` returns and compares it with `MetRemixLoadScreen` through
     * HxStr::MatchesLiteral(). A match pushes `MetRemixLoadScreen`, `MetRemixDataScreen`, and
     * `MetHelpScreen` and activates the first of them. Otherwise it pushes and activates
     * `MetSoloStagesScreen` when MetScreen::mUnknown18 allows, and otherwise records
     * `MetArenasScreen` in the state object and pushes and activates `MetLoadGameScreen`. Every
     * path ends by selecting -1 on the ring.
     *
     * @ghidraAddress 0x001f8658
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the arena buttons, the screenshot material, and the button view. Slot 38.
     *
     * The body is not written. It runs MetScreen::ResolveContainerViews() first, empties the four
     * object vectors, then resolves `as_01.but` and `as_02.but` by name, formats the remaining
     * button names from `as_0%d.but`, appends each to the ring through MetButtonList::Add() with
     * an empty label, and resolves `as_screenshot.mat` and `arena_butts.view`. It ends by
     * allocating the TexturePairRecord at mUnknown90 from the texture names `gArena1.tex` and
     * `gArena2.tex`.
     *
     * @ghidraAddress 0x001f6a08
     */
    virtual void ResolveContainerViews();

private:
    // The arena button ring. The constructor allocates it and the destructor releases it. +0x8c
    MetButtonList *mUnknown8c;
    // The two arena screenshot textures. Slot 38 allocates it and the destructor releases it.
    // +0x90
    TexturePairRecord *mUnknown90;
    // Index of the one entry that is accepted even though it is not below mUnknown98. The
    // constructor writes neither this word nor mUnknown98, so slot 38 fills both. +0x94
    int mUnknown94;
    // Number of real arena entries. Every index below it is a selectable arena. +0x98
    int mUnknown98;
    // The object whose field at `+0x1c` slot 26 hands to the material setter, which is the
    // screenshot material resolved from `as_screenshot.mat`. Its class is not identified. +0x9c
    Rnd::Object *mUnknown9c;
    // The four object vectors slot 38 fills and the destructor deallocates. The element is four
    // bytes and needs no destructor, which the destructor's four plain deallocations fix. The
    // element type is inferred from the vectors of the surrounding Met classes rather than
    // recovered, on the same basis as TexturePairRecord's own vector. +0xa4, +0xb0, +0xbc, +0xc8
    std::vector<Rnd::Object *> mUnknowna4;
    std::vector<Rnd::Object *> mUnknownb0;
    std::vector<Rnd::Object *> mUnknownbc;
    std::vector<Rnd::Object *> mUnknownc8;
};
