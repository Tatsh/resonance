#pragma once

#include "met/metkbuser.h"
#include "met/metloadfreqbasescreen.h"

/**
 * Screen that creates a new FreQ identity and takes its name from the keyboard.
 *
 * `20MetLoadNewFreqScreen` in the RTTI descriptor at `0x008f08d0`, with two public non-virtual
 * bases at fixed offsets, MetLoadFreqBaseScreen at `+0x00` and MetKBUser at `+164`. The 47-entry
 * primary vtable is at `0x007f83a8` and the three-entry MetKBUser table at `0x007f8388` adjusts
 * `this` by `-164`. That table is where this screen supplies the one MetKBUser pure virtual. The
 * primary is the same length as the MetLoadFreqBaseScreen table, so the class declares no virtual
 * of its own.
 *
 * The class declares one data member and the object is therefore at least 0xac bytes rather than
 * the 0xa8 an earlier reading recorded. mUnknowna8 sits above the four bytes of the MetKBUser
 * vptr, and EnterAndShow(), OnKeyboardDismissed(), and OnUnknownSlot2() all address it.
 *
 * The constructor at `0x002a8418` takes only the renderer and the load priority and runs the
 * MetLoadFreqBaseScreen constructor at `0x00291e00`. The destructor at `0x002a8458` restores the
 * primary vptr, restores the MetKBUser vptr to `0x007f16a0`, runs the MetLoadFreqBaseScreen
 * destructor, and releases the object with the tag `MsgSink`.
 *
 * Twelve slots differ from the MetLoadFreqBaseScreen table, and a diff of the two tables reads
 * slots 1, 5, 9, 11, 15, 39, 40, 41, 43, 44, and 45 apart from the type function. This is the only
 * class in the subsystem that fills slot 11, which every other class inherits as a
 * two-instruction stub.
 *
 * The screen labels all three buttons from configuration code 0x258, under the keys `nf_enter`,
 * `nf_edit`, and `nf_create`, and none of the three shows the selected username. The first button
 * opens the keyboard rather than committing a selection, which is the whole difference from
 * MetLoadFreqScreen.
 *
 * Three bodies are not written. OnNameButton() builds a MetKeyboardRequest and passes it to
 * MetKeyboardScreen::Open(), which are both declared now. OnUnknownSlot2() continues past the
 * name assignment into a larger body. OnMsgScreenDismissed() needs members no header in this tree
 * declares yet.
 */
class MetLoadNewFreqScreen : public MetLoadFreqBaseScreen, public MetKBUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002a8418
     */
    MetLoadNewFreqScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002a8458
     */
    virtual ~MetLoadNewFreqScreen();

    /**
     * Set the screen title and the prompt layout, then enter.
     *
     * Slot 5. The title comes from configuration code 0x269 under the key `create_char`, and the
     * prompt layout is `standard_title`. mUnknowna8 is cleared on both paths. The
     * MetLoadFreqBaseScreen body then runs as a direct call.
     *
     * @ghidraAddress 0x002a3890
     */
    virtual void EnterAndShow();

    /**
     * Exit the prompt screen alongside this one, then begin the exit.
     *
     * Slot 9. The prompt screen is exited only while a departure is already recorded in
     * MetScreen::mUnknown18 and the first button is the selected one. The MetScreen body then runs
     * as a direct call on every path.
     *
     * @ghidraAddress 0x002a84c0
     */
    virtual void BeginExit();

    /**
     * Restore this screen after the keyboard closes.
     *
     * Slot 11. A set mUnknowna8 records that OnUnknownSlot2() accepted a name, and the screen is
     * then left as that path arranged it. A clear mUnknowna8 is the user cancelling, and this
     * screen and the prompt screen are pushed again.
     *
     * @ghidraAddress 0x002a3978
     */
    virtual void OnKeyboardDismissed();

    /**
     * Restore this screen after a message screen is dismissed.
     *
     * Slot 15. The body is not written, for the reason recorded in the class documentation.
     *
     * @param name The message screen that was dismissed.
     * @param nChoice The response.
     * @ghidraAddress 0x002a4910
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Write the `nf_enter` label into the first button.
     *
     * Slot 39. Unlike the base and unlike MetLoadFreqScreen, the override shows no username. The
     * label comes from configuration code 0x258 under the key `nf_enter`.
     *
     * @ghidraAddress 0x002a85c0
     */
    virtual void UpdateNameLabel();

    /**
     * Open the keyboard to type a name for the new identity.
     *
     * Slot 40. A MetKeyboardRequest is built with this screen's registry key, the prompt
     * `FreQ name`, an empty initial text, -1 for any controller, and the MetKBUser subobject as the
     * receiver, and is then passed to MetKeyboardScreen::Open().
     *
     * The body is not written, for the reason recorded in the class documentation.
     *
     * @ghidraAddress 0x002a4108
     */
    virtual void OnNameButton();

    /**
     * Hand the selected identity to the FreQ maker.
     *
     * Slot 41. The selected identity goes to MetFreqMakerCanvasScreen::LoadPrefab() with no
     * randomisation, the editing mode is selected through MetFreqMakerButtonsScreen::SetEditing(),
     * MetFreqMakerButtonsScreen::mNewPersona is set, and `MetLoadNewFreqScreen` is recorded in
     * MetFrontEndState::mUnknown24.
     *
     * @ghidraAddress 0x002a3f80
     */
    virtual void PrepareFreqMakerForSelection();

    /**
     * Create a new identity in the FreQ maker.
     *
     * Slot 43. `MetLoadNewFreqScreen` is recorded in MetFrontEndState::mUnknown24, and then
     * MetLoadFreqBaseScreen::OnCreateButton() runs.
     *
     * @ghidraAddress 0x002a8670
     */
    virtual void OnCreateButton();

    /**
     * Take the list of prefabricated identities the FreQ maker offers.
     *
     * Slot 44.
     *
     * @ghidraAddress 0x002a8590
     */
    virtual void AcquireIdentityList();

    /**
     * Rebuild the button ring with a label on each of the three buttons.
     *
     * Slot 45. All three labels come from configuration code 0x258, under the keys `nf_enter`,
     * `nf_edit`, and `nf_create`. UpdateNameLabel() then runs before the prompt list is rebuilt,
     * which overwrites the first label with the same `nf_enter` string. That is what the binary
     * does. The three prompts are the same three the base appends.
     *
     * @ghidraAddress 0x002a3b08
     */
    virtual void BuildButtonList();

    /**
     * Record the name the keyboard committed on the selected identity.
     *
     * MetKBUser slot 2, in the secondary table at `0x007f8388` with a `-164` adjustment. The
     * committed text replaces the selected identity's username, and an empty commit retains the
     * username the identity already had by assigning it over the empty text first. mUnknowna8 is
     * set to 1 before either assignment, which is what OnKeyboardDismissed() reads.
     *
     * The body is not written past the name assignment, for the reason recorded in the class
     * documentation.
     *
     * @param text The text the user entered.
     * @ghidraAddress 0x002a4340
     */
    virtual void OnUnknownSlot2(const HxStr &text);

private:
    // Set by OnUnknownSlot2() when the keyboard commits a name, cleared by EnterAndShow(), and
    // read by OnKeyboardDismissed() to tell a commit from a cancellation. +0xa8
    int mUnknowna8;
};
