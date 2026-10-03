#pragma once

#include "met/metloadfreqbasescreen.h"

#ifndef VIDEO_STANDARD_PAL
#include "memcard/memcarduser.h"
#endif

/**
 * Screen that loads a saved FreQ identity from a memory card.
 *
 * Its RTTI descriptor is at `0x00901f10`. It has two public non-virtual bases at fixed offsets,
 * MetLoadFreqBaseScreen at `+0x00` and MemcardUser at `+164`. The object is 0xac bytes. The
 * 47-entry primary vtable is at `0x007f6fc0` and the 21-entry MemcardUser table at `0x007f6f10`
 * adjusts `this` by `-164`. The primary is the same length as the MetLoadFreqBaseScreen table, and
 * the class declares no new virtual.
 *
 * The constructor at `0x0029bcf0` takes only the renderer and the load priority, runs the
 * MetLoadFreqBaseScreen constructor at `0x00291e00` (it supplies all three names), and writes
 * its two vptrs and mFreqLimitPending. The destructor at `0x0029bd38` restores the primary
 * vptr, restores the MemcardUser vptr to `0x007daf78`, runs the MetLoadFreqBaseScreen destructor,
 * and releases the object with the tag `MsgSink`.
 *
 * Eleven slots differ from the MetLoadFreqBaseScreen table, and a diff of the two tables reads
 * slots 1, 5, 15, 33, 39, 40, 41, 43, 44, and 45 apart from the type function.
 *
 * Unlike the base, this screen labels the second and third buttons. BuildButtonList() looks both
 * labels up through Script::QueryConfigString() under configuration code 0x258, passing the keys
 * `lf_edit` and `lf_create`, and UpdateNameLabel() then appends the selected username to the
 * `lf_edit` label so that the second button reads as an edit of a particular identity.
 *
 * In the European release MetLoadFreqBaseScreen derives from MetMemDetectScreen, and this class
 * has MetLoadFreqBaseScreen as its one base. The object is 0xbc bytes, the 54-entry primary vtable
 * is at `0x0083aec0`, and the MemcardUser table is at `0x0083ae10`. The constructor is at
 * `0x002b94b0` and the destructor at `0x002b94f0`. The class does not override slot 15 and has no
 * data member, and the refusals of OnCreateButton() move to MetLoadFreqBaseScreen.
 */
#ifdef VIDEO_STANDARD_PAL
class MetLoadFreqScreen : public MetLoadFreqBaseScreen {
#else
class MetLoadFreqScreen : public MetLoadFreqBaseScreen, public MemcardUser {
#endif
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x0029bcf0
     * @ghidraAddress PAL: 0x002b94b0
     */
    MetLoadFreqScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x0029bd38
     * @ghidraAddress PAL: 0x002b94f0
     */
    virtual ~MetLoadFreqScreen();

    /**
     * Produce a load screen on the heap.
     *
     * The front end's screen factory at `0x00385180` is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The screen.
     * @ghidraAddress NTSC-U/C: 0x0029bc68
     * @ghidraAddress PAL: 0x002b9428
     */
    static MetScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Set the screen title and the prompt layout, then enter.
     *
     * Slot 5. The title comes from configuration code 0x269 under the key `load_char`, and the
     * prompt layout is `standard_title`. The MetLoadFreqBaseScreen body then runs as a direct
     * call.
     *
     * @ghidraAddress NTSC-U/C: 0x00297448
     * @ghidraAddress PAL: 0x002b5530
     */
    virtual void EnterAndShow();

#ifndef VIDEO_STANDARD_PAL
    /**
     * Restore this screen after the FreQ-limit message is dismissed.
     *
     * Slot 15. A message screen whose name is not `freq_limit` is ignored. The choice the user made
     * is not read, and both responses restore the screen. The title is set again from the same
     * configuration code EnterAndShow() uses.
     *
     * @param name The message screen that was dismissed.
     * @param nChoice The response. This override does not read it.
     * @ghidraAddress NTSC-U/C: 0x00298510
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);
#endif

    /**
     * Play `SND_MET_SELECTFREQ` once the enter animation has finished.
     *
     * Slot 33.
     *
     * @ghidraAddress NTSC-U/C: 0x0029bdc8
     * @ghidraAddress PAL: 0x002b9580
     */
    virtual void OnEnterFinished();

    /**
     * Write the selected username into the first button and the edit label into the second.
     *
     * Slot 39, and slot 45 in the European release. The first button is set exactly as the base
     * sets it. The second takes the `lf_edit` label with the same username appended. The European
     * release instead formats the username into the `EDIT %s` text of the current language.
     *
     * @ghidraAddress NTSC-U/C: 0x002976f8
     * @ghidraAddress PAL: 0x002b5878
     */
    virtual void UpdateNameLabel();

    /**
     * Commit the selected identity to the game manager and advance.
     *
     * Slot 40, and slot 46 in the European release. The game manager's persona list is cleared and
     * the selected identity is added to it, then the game mode decides where the front end goes
     * next, MetNetPortalScreen for mode 3 and MetModeScreen otherwise. MetLeftGizmoScreen is pushed
     * ahead of either.
     *
     * @ghidraAddress NTSC-U/C: 0x002978d0
     * @ghidraAddress PAL: 0x002b5a68
     */
    virtual void OnNameButton();

    /**
     * Hand the selected identity to the FreQ maker.
     *
     * Slot 41, and slot 47 in the European release. The body resolves the canvas and buttons
     * screens, passes the selected MetPersonaData to MetFreqMakerCanvasScreen::LoadPersona(),
     * selects the editing mode through MetFreqMakerButtonsScreen::SetEditing(), clears
     * MetFreqMakerButtonsScreen::mNewPersona, replaces the game manager's persona list with the
     * selected identity, and records `MetLoadFreqScreen` in MetFrontEndState::mReturnScreen.
     *
     * @ghidraAddress NTSC-U/C: 0x00297528
     * @ghidraAddress PAL: 0x002b5648
     */
    virtual void PrepareFreqMakerForSelection();

    /**
     * Refuse a ninth identity, or create one.
     *
     * Slot 43. A list that already has eight or more identities is refused. The help screen
     * is exited, and a message screen named `freq_limit` with one `OK` response shows the limit
     * and the first card slot's name. A first card slot with fewer free clusters than
     * GlobalSettings::mPersonaMinimumFreeClusters is refused the same way, with the
     * `freq_no_space` text, the slot name, and that minimum. OnMsgScreenDismissed() restores this
     * screen after either refusal. Otherwise MetFreqCreateScreen is pushed and activated.
     *
     * The European release is slot 49. It records `MetLoadFreqScreen` in
     * MetFrontEndState::mReturnScreen and runs the MetLoadFreqBaseScreen body as a direct call.
     * Both refusals are made there.
     *
     * @ghidraAddress NTSC-U/C: 0x00297c10
     * @ghidraAddress PAL: 0x002b95a0
     */
    virtual void OnCreateButton();

    /**
     * Take the identity list the memory-card path last read.
     *
     * Slot 44, and slot 51 in the European release.
     *
     * @ghidraAddress NTSC-U/C: 0x0029bda0
     * @ghidraAddress PAL: 0x002b9558
     */
    virtual void AcquireIdentityList();

    /**
     * Rebuild the button ring with labels on the second and third buttons.
     *
     * Slot 45, and slot 52 in the European release. The first button takes an empty label, the
     * second the `lf_edit` label, and the third the `lf_create` label. The three prompts are the
     * same three the base appends.
     *
     * @ghidraAddress NTSC-U/C: 0x00296fe8
     * @ghidraAddress PAL: 0x002b4fe0
     */
    virtual void BuildButtonList();

#ifndef VIDEO_STANDARD_PAL
private:
    // Set to 1 by the constructor, cleared by OnMsgScreenDismissed() once the FreQ limit message
    // closes, and read nowhere in the recovered part of the image. +0xa8
    int mFreqLimitPending;
#endif
};
