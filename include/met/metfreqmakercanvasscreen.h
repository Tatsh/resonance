#pragma once

#include <list>

#include "game/freqappearancedetail.h"
#include "math/color.h"
#include "math/vector2.h"
#include "memcard/memcarduser.h"
#include "met/metscreen.h"
#include "os/hxstr.h"

class FreqPart;
class MetPersonaData;

/**
 * Canvas of the FreQ maker, the screen the avatar under edit is drawn on.
 *
 * `24MetFreqMakerCanvasScreen` in the RTTI descriptor at `0x00902a60`, with two public non-virtual
 * bases at fixed offsets, MetScreen at `+0x00`, and MemcardUser at `+140`.
 *
 * The 39-entry primary vtable is at `0x007f1cd8`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The twenty-one-entry MemcardUser table at `0x007f1c28` adjusts `this` by `-140` in every entry.
 *
 * The object is 0x160 bytes, which the allocation in New() fixes. It embeds the avatar under edit
 * as a FreqAppearanceDetail at `+0xa0`, and the inventory and buttons screens drive that avatar
 * through the members below. Every edit that changes the avatar marks the canvas modified.
 *
 * The translation unit spans `0x0025e5a8` to `0x00262810`. Besides the members below, it has the
 * static initialiser at `0x00261d50`, which builds the default name `player1`, per-unit copies of
 * MsgSink and MemcardUser routines, and template library emissions.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5, 14, 19, 23, 24, 30, 36, and 38.
 */
class MetFreqMakerCanvasScreen : public MetScreen, public MemcardUser {
public:
    /**
     * Construct the screen.
     *
     * Supplies `fm_canvas` for the screen name, `metagame/persona` for the directory, and
     * `freq_maker_canvas` for the container, builds an empty avatar, and names it `player1`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0025e5a8
     */
    MetFreqMakerCanvasScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00262030
     */
    virtual ~MetFreqMakerCanvasScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00261fa8
     */
    static MetFreqMakerCanvasScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Show the screen and hang the avatar view from `fm_canvas.view` again.
     *
     * Slot 5.
     *
     * @ghidraAddress 0x0025e978
     */
    virtual void EnterAndShow();

    /**
     * Report the container load finished once the FreQ maker assets are resident as well.
     *
     * Slot 14.
     *
     * @return Non-zero once both loads have finished.
     * @ghidraAddress 0x002620d8
     */
    virtual int PollContainerLoad();

    /**
     * Ignore the command.
     *
     * Slot 19. The body is empty.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x002620c8
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play nothing.
     *
     * Slot 23. The body is empty.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00261f80
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play nothing.
     *
     * Slot 24. The body is empty.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00261f88
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Do nothing.
     *
     * Slot 30. The body is empty.
     *
     * @param pObject The object slot 29 finished with.
     * @ghidraAddress 0x002620d0
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Unhang the avatar view from `fm_canvas.view`.
     *
     * Slot 36.
     *
     * @ghidraAddress 0x0025e898
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the base views, hang the avatar view from `fm_canvas.view`, and record that it is
     * hung.
     *
     * Slot 38.
     *
     * @ghidraAddress 0x0025e798
     */
    virtual void ResolveContainerViews();

    /**
     * Write the avatar and its name into the persona being edited, or into a new one, and make
     * that persona the game manager's one persona.
     *
     * A new persona also takes the current date and time when they are available, and the copy
     * the game manager keeps becomes the persona being edited. The canvas is then marked
     * unmodified. MetFreqMakerButtonsScreen's slots 15 and 19 call it. The title is inferred.
     *
     * @ghidraAddress 0x0025ea68
     */
    void CommitPersona();

    /**
     * Rename the avatar and show the name on `FREQ_NAME.txt`.
     *
     * The canvas is marked modified. The title is inferred.
     *
     * @param name The new name.
     * @ghidraAddress 0x0025ec80
     */
    void SetFreqName(const HxStr &name);

    /**
     * Start placing a part of one template. The title is inferred.
     *
     * @param name The template name.
     * @ghidraAddress 0x00262120
     */
    void SelectTemplate(const HxStr &name);

    /**
     * Place the previewed part, or stop editing the selected one, and mark the canvas modified.
     * The title is inferred.
     *
     * @ghidraAddress 0x00262140
     */
    void PlaceCursor();

    /**
     * Return the placement state to its unset values. The title is inferred.
     *
     * @ghidraAddress 0x00262170
     */
    void ResetCursor();

    /**
     * Apply one of the canvas editing commands, codes 13 through 21, to the avatar.
     *
     * The inventory screen forwards the commands. The title is inferred.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x00262190
     */
    void HandleCanvasCommand(const MetScreenCommand *pCommand);

    /**
     * Apply a colour to the previewed or selected part. The title is inferred.
     *
     * @param color The colour.
     * @param palettePosition The palette position the colour came from.
     * @ghidraAddress 0x00262250
     */
    void SetColor(const Color &color, const Vector2 &palettePosition);

    /**
     * Report the parts of the avatar. The title is inferred.
     *
     * @return The part list.
     * @ghidraAddress 0x00262270
     */
    std::list<FreqPart *> &GetParts();

    /**
     * Mark the canvas modified and start editing one part. The title is inferred.
     *
     * @param nIndex The part's position in the part list.
     * @return The part, or null for an index outside the list.
     * @ghidraAddress 0x00262290
     */
    FreqPart *SelectPart(int nIndex);

    /**
     * Load one persona's avatar and name for editing.
     *
     * With a null persona the avatar is emptied and named `player1`. The canvas is marked
     * unmodified. The title is inferred.
     *
     * @param pPersona The persona to edit, or null.
     * @ghidraAddress 0x002622b8
     */
    void LoadPersona(MetPersonaData *pPersona);

    /**
     * Start a new persona from a pre-fab one's avatar, optionally randomised.
     *
     * The persona being edited is cleared, the avatar is copied from pSource and named `player1`,
     * and with nRandomize set randomize() runs a random number of times from 0 to 9. The canvas is
     * marked modified. The title is inferred.
     *
     * @param pSource The pre-fab persona.
     * @param nRandomize Non-zero to randomise the copy.
     * @ghidraAddress 0x00262350
     */
    void LoadPrefab(MetPersonaData *pSource, int nRandomize);

    /**
     * Restore the selected part from the copy taken when it was selected. The title is inferred.
     *
     * @ghidraAddress 0x00262440
     */
    void RevertSelection();

    /**
     * Report the avatar's name. The title is inferred.
     *
     * @return The name.
     * @ghidraAddress 0x00262460
     */
    HxStr *GetFreqName();

    /**
     * Delete one part and mark the canvas modified. The title is inferred.
     *
     * @param nIndex The part's position in the part list.
     * @ghidraAddress 0x00262468
     */
    void DeletePart(int nIndex);

    /**
     * Randomise the avatar and mark the canvas modified. The title is inferred.
     *
     * @ghidraAddress 0x00262498
     */
    void Randomize();

    /**
     * Move one part to the origin, bring it to the front, and mark the canvas modified. The title
     * is inferred.
     *
     * @param nIndex The part's position in the part list.
     * @ghidraAddress 0x002624c8
     */
    void RecentrePart(int nIndex);

    // Declared in recovered offset order, with the access specifiers interleaved.

    /**
     * The persona being edited, or null for a new one.
     *
     * Public because MetFreqMakerButtonsScreen's slots 15 and 36 pass it to
     * MetPersonaSaverScreen::StartSave(), and the image has no accessor. +0x90
     */
    MetPersonaData *mPersona;

private:
    unsigned char mUnknown94[0xc];    // +0x94
    FreqAppearanceDetail mAppearance; // +0xa0, the avatar under edit

public:
    /**
     * Non-zero after any edit, and cleared on commit or load.
     *
     * Public because MetFreqMakerButtonsScreen's slot 36 reads it to decide whether to raise the
     * save-before-leaving dialogue, and the image has no accessor. +0x150
     */
    int mModified;

private:
    HxStr mFreqName;   // +0x154
    int mViewAttached; // +0x15c, set once slot 38 has hung the avatar view
};
