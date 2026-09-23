#pragma once

#include <vector>

#include "met/metbuttonlist.h"
#include "met/metpersonadata.h"
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
 * class declares eight virtuals of its own at slots 39 through 46. Every entry of the table was
 * read back byte for byte and agrees with the addresses recorded on the declarations below.
 *
 * Three buttons drive the screen and the interface follows from that. BuildButtonList() appends
 * `cid_01.but`, `cid_02.but`, and `cid_03.but` to mUnknown90 and rebuilds MetScreen::mUnknown38
 * with the prompts `id_name`, `cid_edit`, and `id_create`, in that order. OnUnknownSlot36() then
 * dispatches button 0 to OnNameButton(), button 1 to OnEditButton(), and button 2 to
 * OnCreateButton(), which is what identifies all three. HandleCommand() posts the prompt at the
 * selected index through MetHelpScreen::SetText() on every navigation command.
 *
 * Selecting a button departs the screen before the action runs, and MetScreen::mUnknown18 records
 * which of the two departures is under way. The select command alternates the selected button
 * through MetScreen::StartRepeatingSound(). OnUnknownSlot30() runs once that alternation finishes,
 * writes 2, and starts the exit animation. OnUnknownSlot36() runs once the exit animation
 * finishes, and the 2 sends it to the selected button's action. The back command writes 0 instead,
 * and OnUnknownSlot36() then restores the main menu.
 *
 * The constructor at `0x00291e00` takes only the renderer and the load priority, and supplies
 * `cid` for the screen name, `metagame/_Solo` for the directory, and `create_id` for the
 * container. All three children call it, so all three load the same container and differ only in
 * behaviour. It allocates a MetButtonList tagged `MetButtonList` into mUnknown90, zeroes
 * mUnknown94, waits for the FreQ maker assets, and resolves `persona_texburn_texture_1.tex` into
 * mBurnTexture. mUnknown98 and mUnknown9c are written by ResolveContainerViews() rather than by
 * the constructor.
 *
 * The destructor at `0x00296ae0` restores the vptr, deletes mUnknown90 through slot 1 of the
 * MetButtonList table with the deleting `__in_chrg` value, runs the MetScreen destructor, and
 * releases the object with the tag `MsgSink`.
 *
 * Seven inherited slots differ from the MetScreen table, and all seven bodies are shared by all
 * three children, which is what proves they belong here. Those are slots 5, 19, 23, 24, 30, 36,
 * and 38.
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
     * Populate the screen and enter it.
     *
     * Slot 5. The three build steps run in declaration order, AcquireIdentityList() first, then
     * BuildButtonList(), then UpdateCycleArrows(). A selection index that has run past the end of
     * the new list is reset to the first entry. The prompt for the selected button is posted, the
     * left gizmo screen is pushed, and the MetScreen body then runs as a direct call.
     *
     * @ghidraAddress 0x002926d8
     */
    virtual void EnterAndShow();

    /**
     * Act on one navigation command.
     *
     * Slot 19. A command code outside 1 through 6 is discarded, which is the same six-entry jump
     * table shape MetScreen::DeliverCommand() uses. Left and right are ignored while a button
     * rather than the identity carousel is selected, and select and back both clear the prompt by
     * posting the empty string.
     *
     * @param pCommand The command the renderer translated from an input message.
     * @ghidraAddress 0x00292178
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the cycle-left sound while the carousel is selected and has more than one entry.
     *
     * Slot 23. The override tests neither the selector nor any recorded selector of its own. It
     * forwards to MetScreen with the same selector when MetButtonList::mSelected is zero and the
     * identity list at mUnknown8c has at least kMinimumCyclableEntries entries.
     *
     * @param nSelector Passed through to MetScreen unchanged.
     * @ghidraAddress 0x00296b60
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play the cycle-right sound under the same two conditions as PlayCycleLeftSound().
     *
     * Slot 24.
     *
     * @param nSelector Passed through to MetScreen unchanged.
     * @ghidraAddress 0x00296bb0
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Start the exit animation once the selected button has finished alternating.
     *
     * Slot 30. The two cycle arrows also alternate, through the left and right commands, and a
     * call that reports either of them is ignored so that cycling the carousel does not depart the
     * screen. Any other object is the button the select command alternated, and the departure runs
     * with MetScreen::mUnknown18 at 2 so that OnUnknownSlot36() acts on the button.
     *
     * @param pButton The button slot 29 finished alternating.
     * @ghidraAddress 0x00292c60
     */
    virtual void OnUnknownSlot30(Rnd::Button *pButton);

    /**
     * Act on the departure the exit animation has just finished.
     *
     * Slot 36. MetScreen::mUnknown18 selects between the two halves. A zero is the back command,
     * and the main menu is restored by pushing MetLeftGizmoSmallScreen, MetTopLogoScreen, and
     * MetMainScreen and activating the last. Anything else is a selected button, and the selected
     * index picks one of OnNameButton(), OnEditButton(), and OnCreateButton(). An index outside 0
     * through 2 does nothing, and the button selection is cleared on every path but the back one.
     *
     * @ghidraAddress 0x00292da8
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the two cycle arrows after the container load.
     *
     * Slot 38. The MetScreen body runs first as a direct call. `cid_left.but` and `cid_right.but`
     * are then resolved out of Rnd::g_manager and each cast to Rnd::Button, and a name that
     * resolves to nothing stores a null rather than reporting.
     *
     * @ghidraAddress 0x00292000
     */
    virtual void ResolveContainerViews();

    /**
     * Write the selected identity's username into the first button's label.
     *
     * Slot 39. The username is copy-constructed out of the appearance embedded in the selected
     * MetPersonaData and set on Rnd::Button::mText of button 0. The base body reaches the label
     * through the same path MetButtonList::Add() uses.
     *
     * @ghidraAddress 0x00292620
     */
    virtual void UpdateNameLabel();

    /**
     * Act on the first button, whose prompt is `id_name`.
     *
     * Slot 40. The body is empty here. MetLoadFreqScreen commits the selected identity to the
     * game manager and advances, and MetLoadNewFreqScreen opens the keyboard to type a name
     * instead, which is what fixes the slot as the name action rather than a load.
     *
     * @ghidraAddress 0x00296a50
     */
    virtual void OnNameButton();

    /**
     * Hand the selected identity to the FreQ maker.
     *
     * Slot 41. The body resolves `MetFreqMakerCanvasScreen` and `MetFreqMakerButtonsScreen`
     * through MetScreen::FindScreenByName(), passes the selected MetPersonaData to
     * MetFreqMakerCanvasScreen::LoadPrefab() with no randomisation, selects the editing mode
     * through MetFreqMakerButtonsScreen::SetEditing(), and clears
     * MetFreqMakerButtonsScreen::mNewPersona. OnCreateButton() performs the exact inverse of the
     * last two steps, which separates editing an identity from creating one.
     *
     * @ghidraAddress 0x00293028
     */
    virtual void PrepareFreqMakerForSelection();

    /**
     * Act on the second button, whose prompt is `cid_edit`.
     *
     * Slot 42. PrepareFreqMakerForSelection() runs first, then the four FreQ maker screens are
     * pushed with the buttons screen ahead of the other three, and the buttons screen is
     * activated.
     *
     * @ghidraAddress 0x00293148
     */
    virtual void OnEditButton();

    /**
     * Act on the third button, whose prompt is `id_create`.
     *
     * Slot 43. The body passes a null persona to MetFreqMakerCanvasScreen::LoadPersona(), selects
     * the creating mode through MetFreqMakerButtonsScreen::SetEditing(), sets
     * MetFreqMakerButtonsScreen::mNewPersona, clears the game manager's persona list, then pushes
     * the same four screens with the buttons screen last and activates it.
     *
     * @ghidraAddress 0x002933b8
     */
    virtual void OnCreateButton();

    /**
     * Resolve the list of identities the carousel steps through.
     *
     * Slot 44. The body is empty here and every child supplies mUnknown8c. MetLoadFreqScreen
     * takes MetPersonaData::loadList() and MetLoadNewFreqScreen takes the list the FreQ maker
     * asset manager vends. EnterAndShow() runs it before either of the other two build steps,
     * which is what fixes the order.
     *
     * @ghidraAddress 0x00296d08
     */
    virtual void AcquireIdentityList();

    /**
     * Rebuild the button ring and the prompts that go with it.
     *
     * Slot 45. mUnknown90 is emptied and the three `cid_0N.but` buttons are appended with an empty
     * label each, MetScreen::mUnknown38 is emptied and `id_name`, `cid_edit`, and `id_create` are
     * appended, and the selection is then moved to the first button.
     *
     * @ghidraAddress 0x00292810
     */
    virtual void BuildButtonList();

    /**
     * Show the two cycle arrows only while more than one identity exists.
     *
     * Slot 46. Both arrows are shown or hidden together, and both are put into state 1 when they
     * are shown.
     *
     * @ghidraAddress 0x00296c88
     */
    virtual void UpdateCycleArrows();

protected:
    /**
     * Reapply the selected identity to the preview and the label.
     *
     * The body resolves `cid_char.mat` out of Rnd::g_manager and casts it to Rnd::Mat, runs
     * MetPersonaData::AttachToBurnSlot() on the selected identity with slot 0, assigns
     * mBurnTexture into the material's second stage through Rnd::Mat::Stage::SetTex(), and
     * finishes with UpdateNameLabel(). The title is inferred from those three steps.
     *
     * @ghidraAddress 0x00292508
     */
    void RefreshSelection();

    // The identities the carousel steps through. The element is MetPersonaData: UpdateNameLabel()
    // reads the embedded FreqAppearance at `+0x140` out of an element, and
    // GameManagerImpl::GetPersonas() returns a vector of exactly this element type. No routine of
    // this class writes the member, and both children write it from their AcquireIdentityList(),
    // which is why it is protected. +0x8c
    std::vector<MetPersonaData *> *mUnknown8c;
    // The button ring. Both children rebuild it in their BuildButtonList() and address the label
    // of one button in their UpdateNameLabel(), which is why it is protected. +0x90
    MetButtonList *mUnknown90;
    // Read by MetLoadFreqScreen::OnNameButton() and written by
    // MetLoadNewFreqScreen::OnUnknownSlot11(), which is why it is protected. EnterAndShow() resets
    // it once it has run past the end of a rebuilt list. +0x94
    int mUnknown94;

private:
    /**
     * Step the selection one identity and refresh.
     *
     * HandleCommand() is the one caller, on the left and the right commands. A left command steps
     * back and a right command steps forward, and both wrap.
     *
     * @param pCommand The command that asked for the step.
     * @ghidraAddress 0x00296c00
     */
    void StepSelection(const MetScreenCommand *pCommand);

    // Resolved by ResolveContainerViews() and null while the container has not loaded. +0x98 and
    // +0x9c
    Rnd::Button *mUnknown98;
    Rnd::Button *mUnknown9c;
    // The texture `persona_texburn_texture_1.tex`, resolved by the constructor. +0xa0
    Rnd::Tex *mBurnTexture;
};
