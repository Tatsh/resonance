#pragma once

#include <vector>

#include "met/metkbuser.h"
#include "met/metscreen.h"
#include "os/hxstr.h"

class MetButtonList;
namespace Rnd {
class Button;
}

/**
 * Button row of the FreQ maker.
 *
 * `25MetFreqMakerButtonsScreen` in the RTTI descriptor at `0x00902a50`, with two public non-virtual
 * bases at fixed offsets, MetScreen at `+0x00`, and MetKBUser at `+140`.
 *
 * The 39-entry primary vtable at `0x007f1560` is the same length as the MetScreen table, and the
 * class declares no new virtual. The three-entry MetKBUser table at `0x007f1540` adjusts `this` by
 * `-140` in every entry.
 *
 * New() allocates 0xa0 bytes. The row lists nine buttons, randomise, the five part pages, edit,
 * name, and save, and drives the canvas, directions, and inventory screens of the FreQ maker. The
 * load-identity screens choose between editing an existing persona and creating a new one through
 * SetEditing() and mNewPersona before they bring the row up.
 *
 * The translation unit spans `0x00257968` to `0x0025e5a8`. Besides the members below, it has the
 * type function at `0x0025dfe8`, per-unit copies of MsgSink and MetKBUser routines, and template
 * library emissions.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5, 7, 14, 15, 19, 23, 24, 30, 33, 36, and 38, and MetKBUser slot 2.
 */
class MetFreqMakerButtonsScreen : public MetScreen, public MetKBUser {
public:
    /**
     * Construct the screen.
     *
     * Supplies `fm_buttons` for the screen name, `metagame/persona` for the directory, and
     * `freq_maker_buttons` for the container, allocates the button list, and starts in the edit
     * mode with no pending action.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00257968
     */
    MetFreqMakerButtonsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the button list.
     *
     * @ghidraAddress 0x0025e108
     */
    virtual ~MetFreqMakerButtonsScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x0025e080
     */
    static MetFreqMakerButtonsScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Select the first button, title the row for creating or editing, label the save button, and
     * show the screen.
     *
     * Slot 5. The save button reads `SAVE` when MetFrontEndState::mUnknown0c is set and `DONE`
     * otherwise.
     *
     * @ghidraAddress 0x00258c80
     */
    virtual void EnterAndShow();

    /**
     * Report the container load finished once the FreQ maker assets and the directions and
     * inventory containers are resident as well.
     *
     * Slot 14. Every poll runs, and MetScreen::PollContainerLoad() runs a second time when all of
     * them have finished.
     *
     * @return Non-zero once every load has finished.
     * @ghidraAddress 0x00259fa0
     */
    virtual int PollContainerLoad();

    /**
     * Enable the row again and show the page and directions of the selected button.
     *
     * Slot 7. Every button returns to state 0 and the selected button takes state 1.
     *
     * @ghidraAddress 0x0025e1e0
     */
    virtual void OnUnknownSlot7();

    /**
     * Act on the save-before-leaving dialogue.
     *
     * Slot 15. Only `check_if_changed` is handled. YES commits the persona on the canvas and
     * saves it, returning to the screen MetFrontEndState::mUnknown24 records and the help screen.
     * NO brings back those two screens without saving.
     *
     * @param name The dialogue name.
     * @param nChoice The index of the button chosen.
     * @ghidraAddress 0x00259ad0
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Step the selection, act on the selected button, or exit the FreQ maker.
     *
     * Slot 19. The two ring steps move the selection and show its page and directions. Select
     * opens the keyboard for the name button, commits the persona and exits for the save button,
     * randomises or mutates for the randomise button, and otherwise starts the chosen button's
     * alternation. Back exits without saving.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x002581c8
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play nothing.
     *
     * Slot 23. The body is empty.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0025e070
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play nothing.
     *
     * Slot 24. The body is empty.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0025e078
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Hand the controls to the inventory once the chosen button's alternation has finished.
     *
     * Slot 30. The inventory becomes the active panel, every button is disabled, and the chosen
     * button stays in its pressed state.
     *
     * @param pObject The object whose alternation finished, which is not read.
     * @ghidraAddress 0x00258f40
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Show the page and directions of the selected button.
     *
     * Slot 33.
     *
     * @ghidraAddress 0x0025e1a0
     */
    virtual void OnUnknownSlot33();

    /**
     * Take the action recorded on the way out, once the screen has exited.
     *
     * Slot 36. The inventory pages are hidden first. After the save button the persona is saved
     * and the mode screen, or the network portal in a network game, follows. After a back command
     * the save-before-leaving dialogue is raised when a save is possible and the canvas is
     * modified, and otherwise the screen MetFrontEndState::mUnknown24 records returns.
     *
     * @ghidraAddress 0x00259040
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the base views and add the nine buttons with their labels.
     *
     * Slot 38.
     *
     * @ghidraAddress 0x00257b70
     */
    virtual void ResolveContainerViews();

    /**
     * Take the name the keyboard screen committed.
     *
     * MetKBUser slot 2. A non-empty name goes to MetFreqMakerCanvasScreen::SetFreqName(), and
     * this screen becomes the active panel again. An empty name does nothing.
     *
     * @param text The name.
     * @ghidraAddress 0x0025a108
     */
    virtual void OnUnknownSlot2(const HxStr &text);

    /**
     * Choose between editing an existing persona and creating a new one.
     *
     * Once the container is resident the randomise button is relabelled at once. The
     * load-identity screens and MetLoadPreFabScreen call it before bringing the row up. The title
     * is inferred.
     *
     * @param nEditing 1 to edit, 0 to create.
     * @ghidraAddress 0x0025e288
     */
    void SetEditing(int nEditing);

private:
    // 0x0025a228. Label `RANDOMIZE.txt` with `MUTATE` while editing and `RANDOMIZE` while creating.
    void UpdateRandomizeLabel();

    // 0x0025a3e0. Show the inventory page a part button selects, or hide the pages for the edit,
    // name, randomise, and save buttons. The directions screen is resolved and not used.
    void ShowPageForButton(Rnd::Button *pButton);

    // 0x0025a5e0. Show the directions page for a button, or the blank page for no button.
    void ShowDirectionsForButton(Rnd::Button *pButton);

    // Resolve the canvas and hand its persona to MetPersonaSaverScreen::StartSave(). The card is
    // the one GlobalSettings records when MetFrontEndState::mUnknown0c is set, and otherwise a
    // stand-in slot named `1` on port 0. Slots 15 and 36 expand it.
    void StartPersonaSave(const std::vector<HxStr> &screens);

    // Declared in recovered offset order, with the access specifiers interleaved.

    MetButtonList *mButtonList; // +0x90
    // 1 while editing an existing persona, 0 while creating one. +0x94
    int mEditing;

public:
    /**
     * Non-zero while the persona under edit has not been saved before.
     *
     * MetLoadFreqBaseScreen sets it when creating and clears it when editing, and the save path
     * passes it to MetPersonaSaverScreen::StartSave(). +0x98
     */
    int mNewPersona;

private:
    // The action slot 36 takes once the screen has exited, from ExitAction. +0x9c
    int mExitAction;
};
