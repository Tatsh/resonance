#pragma once

#include <vector>

#include "met/metloadfreqbasescreen.h"

class MetPersonaData;

/**
 * Screen that picks one of the pre-built FreQ identities.
 *
 * `19MetLoadPreFabScreen` in the RTTI descriptor at `0x008ef4f0`, with MetLoadFreqBaseScreen as
 * its one public non-virtual base at offset 0. The object is 0xb0 bytes and the 47-entry vtable at
 * `0x007f8c28` is the same length as the MetLoadFreqBaseScreen table, so the class declares no
 * virtual of its own.
 *
 * The constructor at `0x002a8a58` takes only the renderer and the load priority, runs the
 * MetLoadFreqBaseScreen constructor at `0x00291e00`, writes its own vptr, and empties the one
 * vector below. The destructor at `0x002ad450` restores the vptr, returns the vector buffer to the
 * pool, runs the MetLoadFreqBaseScreen destructor, and releases the object with the tag `MsgSink`.
 *
 * Ten slots differ from the MetLoadFreqBaseScreen table. Apart from the destructor and slot 41,
 * PrepareFreqMakerForSelection(), they are 5 `0x002a8aa0`, 15 `0x002a9b38`, 39 `0x002a8fe0`, 40
 * `0x002a94f0`, 43 `0x002a9710`, 44 `0x002a91b8`, and 45 `0x002a8b80`.
 */
class MetLoadPreFabScreen : public MetLoadFreqBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002a8a58
     */
    MetLoadPreFabScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002ad450
     */
    virtual ~MetLoadPreFabScreen();

    /**
     * Produce a pre-fab screen on the heap.
     *
     * The front end's screen factory at `0x00385180` is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The screen.
     * @ghidraAddress 0x002ad3c8
     */
    static MetScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Select the prompt layout and title, then show the screen.
     *
     * Slot 5. The prompt layout is `standard_title` and the title `pick_char`.
     *
     * @ghidraAddress 0x002a8aa0
     */
    virtual void EnterAndShow();

    /**
     * Return from the FreQ limit dialogue.
     *
     * Slot 15. Only the `freq_limit` dialogue is handled, whichever button closed it. The title is
     * restored, and the help screen and this screen are pushed again.
     *
     * @param name The dialogue name.
     * @param nChoice The button that closed the dialogue, ignored.
     * @ghidraAddress 0x002a9b38
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Show the selected identity's name.
     *
     * Slot 39. The name button shows the name, and the edit button shows `pf_edit` followed by it.
     *
     * @ghidraAddress 0x002a8fe0
     */
    virtual void UpdateNameLabel();

    /**
     * Play as the selected identity.
     *
     * Slot 40. The identity becomes the game manager's only persona, and the left gizmo and mode
     * select screens are pushed. Reaching the screen in net mode is a fatal error.
     *
     * @ghidraAddress 0x002a94f0
     */
    virtual void OnNameButton();

    /**
     * Hand the selected identity to the FreQ maker.
     *
     * Slot 41. An index inside MetPersonaData::savedList() is a saved persona and goes to
     * MetFreqMakerCanvasScreen::LoadPersona(), and a later index is a pre-fab and goes to
     * MetFreqMakerCanvasScreen::LoadPrefab() with no randomisation. The editing mode is then
     * selected through MetFreqMakerButtonsScreen::SetEditing(),
     * MetFreqMakerButtonsScreen::mNewPersona is cleared, and `MetLoadPreFabScreen` is recorded in
     * MetFrontEndState::mUnknown24.
     *
     * @ghidraAddress 0x002a9330
     */
    virtual void PrepareFreqMakerForSelection();

    /**
     * Create an identity, unless the saved list is full.
     *
     * Slot 43. With eight saved personas the help screen departs and the `freq_limit` dialogue
     * shows `nomem_freq_limit`. Otherwise `MetLoadPreFabScreen` is recorded in
     * MetFrontEndState::mUnknown24 and MetLoadFreqBaseScreen::OnCreateButton() runs.
     *
     * @ghidraAddress 0x002a9710
     */
    virtual void OnCreateButton();

    /**
     * Offer the saved personas followed by the pre-fab identities.
     *
     * Slot 44. mIdentities is refilled and becomes the identity list.
     *
     * @ghidraAddress 0x002a91b8
     */
    virtual void AcquireIdentityList();

    /**
     * Build the name, edit, and create buttons and their prompts.
     *
     * Slot 45. The name button has an empty label and the other two read `pf_edit` and
     * `pf_create`. The name button is selected.
     *
     * @ghidraAddress 0x002a8b80
     */
    virtual void BuildButtonList();

private:
    // The saved personas followed by the pre-fab identities, which AcquireIdentityList() offers.
    std::vector<MetPersonaData *> mIdentities; // +0xa4
};
