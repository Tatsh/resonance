#pragma once

#include <vector>

#include "met/metbuttonlist.h"
#include "met/metpersonadata.h"
#include "met/metscreen.h"
#include "rnd/button.h"
#include "rnd/object.h"
#include "rnd/tex.h"

#ifdef VIDEO_STANDARD_PAL
#include "met/metmemdetectscreen.h"
#endif

/**
 * Base of the three screens that pick a saved FreQ identity.
 *
 * Its RTTI descriptor is at `0x008f2a30`. It has MetScreen as its one public non-virtual base at
 * offset 0. The object is 0xa4 bytes, and two children fix the size independently,
 * MetLoadFreqScreen by placing MemcardUser at `+164` and MetLoadNewFreqScreen by placing MetKBUser
 * at `+164`.
 *
 * The European release derives the class from MetMemDetectScreen instead, with the descriptor at
 * `0x00937a40`. The object is 0xbc bytes there, and the two children place their second base at
 * `+188`. MetLoadFreqScreen then has no second base, and it accesses MemcardUser through
 * MetMemDetectScreen.
 *
 * Three classes derive from the class: MetLoadFreqScreen, MetLoadNewFreqScreen, and
 * MetLoadPreFabScreen.
 *
 * The primary vtable at `0x007f6700` has 47 entries, eight more than the MetScreen table. The
 * class declares eight new virtuals at slots 39 through 46. Every entry of the table was
 * read back byte for byte and agrees with the addresses recorded on the declarations below. The
 * European table at `0x0083a678` has 54 entries. Slots 39 through 44 are the MetMemDetectScreen
 * virtuals, and the eight virtuals of this class follow at slots 45 through 53, with
 * OpenFreqMakerForCreate() added as slot 50. The MemcardUser table is at `0x0083a5c8`.
 *
 * Three buttons drive the screen. BuildButtonList() appends
 * `cid_01.but`, `cid_02.but`, and `cid_03.but` to mButtonList and rebuilds MetScreen::mHelpKeys
 * with the prompts `id_name`, `cid_edit`, and `id_create`, in that order. OnExitFinished() then
 * dispatches button 0 to OnNameButton(), button 1 to OnEditButton(), and button 2 to
 * OnCreateButton(). The dispatch order identifies all three handlers. HandleCommand() posts the
 * prompt at the selected index through MetHelpScreen::SetText() on every navigation command.
 *
 * Selecting a button departs the screen before the action runs, and MetScreen::mExitChoice records
 * which of the two departures is under way. The select command alternates the selected button
 * through MetScreen::StartRepeatingSound(). OnRepeatingSoundFinished() runs once that alternation
 * finishes, writes 2, and starts the exit animation. OnExitFinished() runs once the exit animation
 * finishes, and the 2 sends it to the selected button's action. The back command writes 0 instead,
 * and OnExitFinished() then restores the main menu.
 *
 * The constructor at `0x00291e00` takes only the renderer and the load priority, and supplies
 * `cid` for the screen name, `metagame/_Solo` for the directory, and `create_id` for the
 * container. All three children call it, load the same container, and differ only in
 * behaviour. It allocates a MetButtonList tagged `MetButtonList` into mButtonList, zeroes
 * mSelectedIdentity, waits for the FreQ maker assets, and resolves
 * `persona_texburn_texture_1.tex` into mBurnTexture. mLeftArrow and mRightArrow are written by
 * ResolveContainerViews() rather than by the constructor.
 *
 * The destructor at `0x00296ae0` restores the vptr, deletes mButtonList through slot 1 of the
 * MetButtonList table with the deleting `__in_chrg` value, runs the MetScreen destructor, and
 * releases the object with the tag `MsgSink`.
 *
 * Seven inherited slots differ from the MetScreen table, and all seven bodies are shared by all
 * three children. They are slots 5, 19, 23, 24, 30, 36,
 * and 38. The European release also overrides slots 15, 39, 41, and 42.
 *
 * In the European release, creating an identity from MetLoadFreqScreen checks the memory card in
 * MEMORY CARD slot 1 first. OnCreateButton() refuses a ninth identity or a card without room with a
 * `freq_limit` dialogue offering RETRY and CONTINUE. RETRY probes the cards again through
 * StartDetect(), and OnDetectFinished() then runs OnCreateButton() again while a card is in use.
 */
#ifdef VIDEO_STANDARD_PAL
class MetLoadFreqBaseScreen : public MetMemDetectScreen {
#else
class MetLoadFreqBaseScreen : public MetScreen {
#endif
public:
    /**
     * Construct the screen.
     *
     * The European release also sets mUsingMemcardOnEnter to 1.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x00291e00
     * @ghidraAddress PAL: 0x002add68
     */
    MetLoadFreqBaseScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x00296ae0
     * @ghidraAddress PAL: 0x002b4a40
     */
    virtual ~MetLoadFreqBaseScreen();

    /**
     * Produce a base load screen on the heap.
     *
     * No call site exists.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The screen.
     * @ghidraAddress NTSC-U/C: 0x00296a58
     * @ghidraAddress PAL: 0x002b49b8
     */
    static MetScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Populate the screen and enter it.
     *
     * Slot 5. The three build steps run in declaration order, AcquireIdentityList() first, then
     * BuildButtonList(), then UpdateCycleArrows(). A selection index that has run past the end of
     * the new list is reset to the first entry. The prompt for the selected button is posted, the
     * left gizmo screen is pushed, and the MetScreen body then runs as a direct call. The European
     * release first records MetFrontEndState::mUsingMemcard in mUsingMemcardOnEnter.
     *
     * @ghidraAddress NTSC-U/C: 0x002926d8
     * @ghidraAddress PAL: 0x002ae7d8
     */
    virtual void EnterAndShow();

#ifdef VIDEO_STANDARD_PAL
    /**
     * Act on the response to a `freq_limit` or `mem_no_cards` dialogue.
     *
     * Slot 15. The first button of either dialogue, RETRY, probes the cards again through
     * StartDetect(). CONTINUE on `freq_limit` restores this screen with the `LOAD YOUR FREQ` title.
     * CANCEL on `mem_no_cards` removes this screen from the renderer and restores the main menu.
     * Any other dialogue goes to MetMemDetectScreen.
     *
     * @param name The message screen that was dismissed.
     * @param nChoice The chosen button, counted from zero.
     * @ghidraAddress PAL: 0x002b0840
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);
#endif

    /**
     * Act on one navigation command.
     *
     * Slot 19. A command code outside 1 through 6 is discarded. The six-entry jump table has the
     * shape of the one MetScreen::DeliverCommand() uses. Left and right are ignored while a button
     * rather than the identity carousel is selected, and select and back both clear the prompt by
     * posting the empty string.
     *
     * @param pCommand The command the renderer translated from an input message.
     * @ghidraAddress NTSC-U/C: 0x00292178
     * @ghidraAddress PAL: 0x002ae1a0
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the cycle-left sound while the carousel is selected and has more than one entry.
     *
     * Slot 23. The override tests neither the selector nor any recorded selector. It
     * forwards to MetScreen with the same selector when MetButtonList::mSelected is zero and the
     * identity list at mIdentityList has at least kMinimumCyclableEntries entries.
     *
     * @param nSelector Passed through to MetScreen unchanged.
     * @ghidraAddress NTSC-U/C: 0x00296b60
     * @ghidraAddress PAL: 0x002b4ac8
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play the cycle-right sound under the same two conditions as PlayCycleLeftSound().
     *
     * Slot 24.
     *
     * @param nSelector Passed through to MetScreen unchanged.
     * @ghidraAddress NTSC-U/C: 0x00296bb0
     * @ghidraAddress PAL: 0x002b4b18
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Start the exit animation once the selected button has finished alternating.
     *
     * Slot 30. The two cycle arrows also alternate, through the left and right commands, and a
     * call that reports either of them is ignored so that cycling the carousel does not depart the
     * screen. Any other object is the button the select command alternated, and the departure runs
     * with MetScreen::mExitChoice at 2 so that OnExitFinished() acts on the button.
     *
     * @param pButton The button slot 29 finished alternating.
     * @ghidraAddress NTSC-U/C: 0x00292c60
     * @ghidraAddress PAL: 0x002aee98
     */
    virtual void OnRepeatingSoundFinished(Rnd::Button *pButton);

    /**
     * Act on the departure the exit animation has just finished.
     *
     * Slot 36. MetScreen::mExitChoice selects between the two halves. A zero is the back command,
     * and the main menu is restored by pushing MetLeftGizmoSmallScreen, MetTopLogoScreen, and
     * MetMainScreen and activating the last. Anything else is a selected button, and the selected
     * index picks one of OnNameButton(), OnEditButton(), and OnCreateButton(). An index outside 0
     * through 2 does not call a handler, and the button selection is cleared on every path.
     *
     * @ghidraAddress NTSC-U/C: 0x00292da8
     * @ghidraAddress PAL: 0x002af020
     */
    virtual void OnExitFinished();

    /**
     * Resolve the two cycle arrows after the container load.
     *
     * Slot 38. The MetScreen body runs first as a direct call. `cid_left.but` and `cid_right.but`
     * are then resolved out of Rnd::g_manager and each cast to Rnd::Button, and a name that
     * resolves to nothing stores a null rather than reporting.
     *
     * @ghidraAddress NTSC-U/C: 0x00292000
     * @ghidraAddress PAL: 0x002adfe0
     */
    virtual void ResolveContainerViews();

#ifdef VIDEO_STANDARD_PAL
    /**
     * Show the `mem_load` notice and list the connected cards.
     *
     * Slot 39. The notice has the `WARNING` title, the text that checks for a memory card in MEMORY
     * CARD slot 1, and no buttons. The screen is added back to the renderer, and the
     * MetMemDetectScreen body then runs as a direct call.
     *
     * @ghidraAddress PAL: 0x002b11e0
     */
    virtual void StartDetect();

    /**
     * Report that MEMORY CARD slot 1 has no usable card.
     *
     * Slot 41. Raises `mem_no_cards` with RETRY and CANCEL, the `WARNING` title, and the text that
     * reports no memory card in MEMORY CARD slot 1.
     *
     * @ghidraAddress PAL: 0x002b0d98
     */
    virtual void OnNoCard();

    /**
     * Retry the create action once the probe ends.
     *
     * Slot 42. The screen is removed from the renderer, and MetFrontEndState::mUsingMemcard is
     * restored when mUsingMemcardOnEnter is 1. OnCreateButton() then runs again while
     * MetFrontEndState::mUsingMemcard is set.
     *
     * @ghidraAddress PAL: 0x002b4c78
     */
    virtual void OnDetectFinished();
#endif

    /**
     * Write the selected identity's username into the first button's label.
     *
     * Slot 39, and slot 45 in the European release. The username is copy-constructed out of the
     * appearance embedded in the selected MetPersonaData and set on Rnd::Button::mText of button 0.
     * The base body accesses the label through the same path MetButtonList::Add() uses.
     *
     * @ghidraAddress NTSC-U/C: 0x00292620
     * @ghidraAddress PAL: 0x002ae700
     */
    virtual void UpdateNameLabel();

    /**
     * Act on the first button, whose prompt is `id_name`.
     *
     * Slot 40, and slot 46 in the European release. The body is empty here. MetLoadFreqScreen
     * commits the selected identity to the game manager and advances, and MetLoadNewFreqScreen
     * opens the keyboard to type a name instead. The difference fixes the slot as the name action
     * rather than a load.
     *
     * @ghidraAddress NTSC-U/C: 0x00296a50
     * @ghidraAddress PAL: 0x002b49b0
     */
    virtual void OnNameButton();

    /**
     * Hand the selected identity to the FreQ maker.
     *
     * Slot 41, and slot 47 in the European release. The body resolves `MetFreqMakerCanvasScreen`
     * and `MetFreqMakerButtonsScreen` through MetScreen::FindScreenByName(), passes the selected
     * MetPersonaData to MetFreqMakerCanvasScreen::LoadPrefab() with no randomisation, selects the
     * editing mode through MetFreqMakerButtonsScreen::SetEditing(), and clears
     * MetFreqMakerButtonsScreen::mNewPersona. OnCreateButton() performs the exact inverse of the
     * last two steps. The two steps separate editing an identity from creating one.
     *
     * @ghidraAddress NTSC-U/C: 0x00293028
     * @ghidraAddress PAL: 0x002af320
     */
    virtual void PrepareFreqMakerForSelection();

    /**
     * Act on the second button, whose prompt is `cid_edit`.
     *
     * Slot 42, and slot 48 in the European release. PrepareFreqMakerForSelection() runs first, then
     * the four FreQ maker screens are pushed with the buttons screen ahead of the other three, and
     * the buttons screen is activated.
     *
     * @ghidraAddress NTSC-U/C: 0x00293148
     * @ghidraAddress PAL: 0x002af488
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
     * The European release moves that body to OpenFreqMakerForCreate(), and the slot is 49. The
     * body there refuses the action while MetFrontEndState::mReturnScreen is `MetLoadFreqScreen`
     * and eight or more identities exist, or while the first card slot GlobalSettings records has
     * fewer free clusters than GlobalSettings::mPersonaMinimumFreeClusters. Either refusal exits
     * the help screen and raises `freq_limit` with RETRY and CONTINUE and the `ERROR` title.
     * Otherwise the help screen and MetFreqCreateScreen are pushed and the latter is activated.
     *
     * @ghidraAddress NTSC-U/C: 0x002933b8
     * @ghidraAddress PAL: 0x002af798
     */
    virtual void OnCreateButton();

#ifdef VIDEO_STANDARD_PAL
    /**
     * Open the FreQ maker to create an identity.
     *
     * Slot 50. The body is the North American OnCreateButton() body. MetLoadNewFreqScreen and
     * MetLoadPreFabScreen call it from their OnCreateButton(). The title is inferred.
     *
     * @ghidraAddress PAL: 0x002b0418
     */
    virtual void OpenFreqMakerForCreate();
#endif

    /**
     * Resolve the list of identities the carousel steps through.
     *
     * Slot 44, and slot 51 in the European release. The body is empty here and every child
     * supplies mIdentityList. MetLoadFreqScreen takes MetPersonaData::loadList() and
     * MetLoadNewFreqScreen takes the list the FreQ maker asset manager vends. EnterAndShow() runs
     * it before either of the other two build steps.
     *
     * @ghidraAddress NTSC-U/C: 0x00296d08
     * @ghidraAddress PAL: 0x002b4c70
     */
    virtual void AcquireIdentityList();

    /**
     * Rebuild the button ring and the prompts that go with it.
     *
     * Slot 45, and slot 52 in the European release. mButtonList is emptied and the three
     * `cid_0N.but` buttons are appended with an empty label each, MetScreen::mHelpKeys is emptied
     * and `id_name`, `cid_edit`, and `id_create` are appended, and the selection is then moved to
     * the first button.
     *
     * @ghidraAddress NTSC-U/C: 0x00292810
     * @ghidraAddress PAL: 0x002ae940
     */
    virtual void BuildButtonList();

    /**
     * Show the two cycle arrows only while more than one identity exists.
     *
     * Slot 46, and slot 53 in the European release. Both arrows are shown or hidden together, and
     * both are put into state 1 when they are shown.
     *
     * @ghidraAddress NTSC-U/C: 0x00296c88
     * @ghidraAddress PAL: 0x002b4bf0
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
     * @ghidraAddress NTSC-U/C: 0x00292508
     * @ghidraAddress PAL: 0x002ae5c8
     */
    void RefreshSelection();

    // The identities the carousel steps through. The element is MetPersonaData. UpdateNameLabel()
    // reads the embedded FreqAppearance at `+0x140` out of an element, and
    // GameManagerImpl::GetPersonas() returns a vector of exactly this element type. No routine of
    // this class writes the member. It is protected for the two children that write it from
    // their AcquireIdentityList(). +0x8c, +0xa0 in the European release
    std::vector<MetPersonaData *> *mIdentityList;
    // The button ring. It is protected for the two children that rebuild it in their
    // BuildButtonList() and address the label of one button in their UpdateNameLabel(). +0x90,
    // +0xa4 in the European release
    MetButtonList *mButtonList;
    // The index of the selected identity in mIdentityList. Read by
    // MetLoadFreqScreen::OnNameButton() and MetLoadNewFreqScreen::OnKeyboardTextEntered(), and
    // protected for those two readers. EnterAndShow() resets it once it has run past the end of a
    // rebuilt list. +0x94, +0xa8 in the European release
    int mSelectedIdentity;

private:
    /**
     * Step the selection one identity and refresh.
     *
     * HandleCommand() is the one caller, on the left and the right commands. A left command steps
     * back and a right command steps forward, and both wrap.
     *
     * @param pCommand The command that requested the step.
     * @ghidraAddress NTSC-U/C: 0x00296c00
     * @ghidraAddress PAL: 0x002b4b68
     */
    void StepSelection(const MetScreenCommand *pCommand);

    // The cycle arrows `cid_left.but` and `cid_right.but`, resolved by ResolveContainerViews() and
    // null while the container has not loaded. +0x98 and +0x9c, +0xac and +0xb0 in the European
    // release
    Rnd::Button *mLeftArrow;
    Rnd::Button *mRightArrow;
    // The texture `persona_texburn_texture_1.tex`, resolved by the constructor. +0xa0, +0xb4 in the
    // European release
    Rnd::Tex *mBurnTexture;
#ifdef VIDEO_STANDARD_PAL
    // MetFrontEndState::mUsingMemcard as EnterAndShow() found it, and 1 until then. +0xb8
    int mUsingMemcardOnEnter;
#endif
};
