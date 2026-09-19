#pragma once

#include "memcard/memcarduser.h"
#include "met/metloadfreqbasescreen.h"

/**
 * Screen that loads a saved FreQ identity from a memory card.
 *
 * `17MetLoadFreqScreen` in the RTTI descriptor at `0x00901f10`, with two public non-virtual bases
 * at fixed offsets, MetLoadFreqBaseScreen at `+0x00` and MemcardUser at `+164`. The object is 0xac
 * bytes. The 47-entry primary vtable is at `0x007f6fc0` and the 21-entry MemcardUser table at
 * `0x007f6f10` adjusts `this` by `-164`. The primary is the same length as the
 * MetLoadFreqBaseScreen table, so the class declares no virtual of its own.
 *
 * The constructor at `0x0029bcf0` takes only the renderer and the load priority, runs the
 * MetLoadFreqBaseScreen constructor at `0x00291e00`, which supplies all three names, and writes
 * its own two vptrs and mUnknowna8. The destructor at `0x0029bd38` restores the primary vptr,
 * restores the MemcardUser vptr to `0x007daf78`, runs the MetLoadFreqBaseScreen destructor, and
 * releases the object with the tag `MsgSink`.
 *
 * Eleven slots differ from the MetLoadFreqBaseScreen table, and a diff of the two tables reads
 * slots 1, 5, 15, 33, 39, 40, 41, 43, 44, and 45 apart from the type function.
 *
 * Unlike the base, this screen labels the second and third buttons. BuildButtonList() looks both
 * labels up through Script::QueryConfigString() under configuration code 0x258, passing the keys
 * `lf_edit` and `lf_create`, and UpdateNameLabel() then appends the selected username to the
 * `lf_edit` label so that the second button reads as an edit of a particular identity.
 *
 * Two bodies are not written. PrepareFreqMakerForSelection() calls MetFreqMakerCanvasScreen and
 * MetFreqMakerButtonsScreen members that no header in this tree declares yet, at `0x002622b8` and
 * `0x0025e288`, and it continues into a further unrecovered routine at `0x00217f30`.
 * OnCreateButton() builds a message screen, and the MetMsgScreen construction it needs is not
 * recovered.
 */
class MetLoadFreqScreen : public MetLoadFreqBaseScreen, public MemcardUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0029bcf0
     */
    MetLoadFreqScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0029bd38
     */
    virtual ~MetLoadFreqScreen();

    /**
     * Set the screen title and the prompt layout, then enter.
     *
     * Slot 5. The title comes from configuration code 0x269 under the key `load_char`, and the
     * prompt layout is `standard_title`. The MetLoadFreqBaseScreen body then runs as a direct
     * call.
     *
     * @ghidraAddress 0x00297448
     */
    virtual void EnterAndShow();

    /**
     * Restore this screen after the FreQ-limit message is dismissed.
     *
     * Slot 15. A message screen whose name is not `freq_limit` is ignored, and the choice the user
     * made is not read, so both responses restore the screen. The title is set again from the same
     * configuration code EnterAndShow() uses.
     *
     * @param name The message screen that was dismissed.
     * @param nChoice The response, which this override does not read.
     * @ghidraAddress 0x00298510
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Play `SND_MET_SELECTFREQ` once the enter animation has finished.
     *
     * Slot 33.
     *
     * @ghidraAddress 0x0029bdc8
     */
    virtual void OnUnknownSlot33();

    /**
     * Write the selected username into the first button and the edit label into the second.
     *
     * Slot 39. The first button is set exactly as the base sets it. The second takes the `lf_edit`
     * label with the same username appended.
     *
     * @ghidraAddress 0x002976f8
     */
    virtual void UpdateNameLabel();

    /**
     * Commit the selected identity to the game manager and advance.
     *
     * Slot 40. The game manager's persona list is cleared and the selected identity is added to
     * it, then the game mode decides where the front end goes next, MetNetPortalScreen for mode 3
     * and MetModeScreen otherwise. MetLeftGizmoScreen is pushed ahead of either.
     *
     * @ghidraAddress 0x002978d0
     */
    virtual void OnNameButton();

    /**
     * Hand the selected identity to the FreQ maker.
     *
     * Slot 41. The body resolves the canvas and buttons screens, passes the selected
     * MetPersonaData to the canvas member at `0x002622b8`, runs the buttons member at `0x0025e288`
     * with 1, clears the buttons word at `+0x98`, clears the game manager's persona list, adds the
     * selected identity to it, and then continues through `0x00217f30` and pushes this screen
     * again by name.
     *
     * The body is not written, for the reason recorded in the class documentation.
     *
     * @ghidraAddress 0x00297528
     */
    virtual void PrepareFreqMakerForSelection();

    /**
     * Refuse a ninth identity, or create one.
     *
     * Slot 43. A list already holding eight or more identities is refused. The prompt layout is
     * set to `MetHelpScreen`, a message screen is built with the name `freq_limit` and one `OK`
     * response, and OnMsgScreenDismissed() restores this screen when the user dismisses it. The
     * entry limit is the one hard number the class records.
     *
     * The body is not written, for the reason recorded in the class documentation.
     *
     * @ghidraAddress 0x00297c10
     */
    virtual void OnCreateButton();

    /**
     * Take the identity list the memory-card path last read.
     *
     * Slot 44.
     *
     * @ghidraAddress 0x0029bda0
     */
    virtual void AcquireIdentityList();

    /**
     * Rebuild the button ring with labels on the second and third buttons.
     *
     * Slot 45. The first button takes an empty label, the second the `lf_edit` label, and the
     * third the `lf_create` label. The three prompts are the same three the base appends.
     *
     * @ghidraAddress 0x00296fe8
     */
    virtual void BuildButtonList();

private:
    // Cleared by OnMsgScreenDismissed() and by the constructor, and read nowhere in the recovered
    // part of the image. +0xa8
    int mUnknowna8;
};
