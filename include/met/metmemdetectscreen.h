#pragma once

#include "memcard/memcarduser.h"
#include "met/metscreen.h"

/**
 * Base of the screens that probe for a memory card before using one.
 *
 * `19MetMemDetectScreen` in the RTTI descriptor at `0x008eff50`, with two public non-virtual bases
 * at fixed offsets, MetScreen at `+0x00` and MemcardUser at `+140`. The object is 0xa0 bytes, and
 * two children fix that independently, MetMemDetectStartup by placing FadeUser at `+160` and
 * MetMemCardLoadScreen by placing MetMemCardPickerUser at `+160`.
 *
 * Three classes derive from the class, MetLocPickCharScreen, MetMemCardLoadScreen, and
 * MetMemDetectStartup.
 *
 * Two vtables belong to the class, the 44-entry primary at `0x007fccb8` and the 21-entry
 * MemcardUser table at `0x007fcc08` that adjusts `this` by `-140` in every entry. The primary is
 * five entries longer than the MetScreen table. The five new virtuals at slots 39 through 43 are
 * declared below in slot order.
 *
 * The probe runs as a chain of memory-card tasks. StartDetect() lists the connected cards,
 * OnAllConnectStates() loads the global settings from the card in port 1 or offers to format it,
 * OnGlobalSettingsLoaded() starts StartLoadPersonas(), OnPersonasLoaded() starts
 * StartSaveSpaceCheck(), and OnMinimumSaveSpace() either warns about the free space or shows the
 * autosave notice. Each dialogue returns through OnMsgScreenDismissed(). That routine retries
 * through StartDetect() or ends the probe through OnDetectFinished(). MetFrontEndState::mUnknown0c
 * records whether the probe settled on a card, 1 when it did and 0 when the player continued
 * without one.
 *
 * The translation unit spans `0x002d89c8` to `0x002df058`. Besides the members below, it has the
 * type function at `0x002dea28`, per-unit copies of MsgSink and MemcardUser routines, and template
 * library emissions.
 */
class MetMemDetectScreen : public MetScreen, public MemcardUser {
public:
    /**
     * Construct the detector.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @param name The screen name.
     * @param directory The directory the container loads from.
     * @param file The container name, without the `.rnd` suffix.
     * @ghidraAddress 0x002deab8
     */
    MetMemDetectScreen(MetRenderer *pRenderer,
                       int nPriority,
                       const HxStr &name,
                       const HxStr &directory,
                       const HxStr &file);

    /**
     * @ghidraAddress 0x002deb08
     */
    virtual ~MetMemDetectScreen();

    /**
     * Build a detector on the heap with an empty screen name, directory, and container name.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002d89c8
     */
    static MetMemDetectScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Act on the player's response to one of the probe's dialogues.
     *
     * Slot 15. A retry runs StartDetect(), and continuing ends the probe through
     * OnDetectFinished(). Confirming `mem_format_check` formats the card in port 1. Dismissing
     * `format_success` records the formatted card's free space and shows the autosave notice. A
     * dialogue the screen does not recognise ends the probe.
     *
     * @param name The message screen that was dismissed.
     * @param nChoice The chosen button, counted from zero.
     * @ghidraAddress 0x002d9e40
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Close the autosave notice once it has been up for 480 units of renderer time.
     *
     * Slot 26.
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress 0x002dec10
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Load the global settings from the card in port 1, or offer to format it.
     *
     * MemcardUser slot 3. OnNoCard() runs instead when no card is listed or the first card listed
     * is not in port 1.
     *
     * @ghidraAddress 0x002d8e98
     */
    virtual void OnAllConnectStates();

    /**
     * Warn when the card in port 1 has less free space than a save needs, or show the autosave
     * notice.
     *
     * MemcardUser slot 4.
     *
     * @param nPortSlot The card the check ran on. The body does not read it.
     * @param nSpace The free clusters a save needs.
     * @ghidraAddress 0x002da868
     */
    virtual void OnMinimumSaveSpace(int nPortSlot, int nSpace);

    /**
     * Report the result of formatting the card in port 1.
     *
     * MemcardUser slot 5.
     *
     * @param nPortSlot The card that was formatted. The body does not read it.
     * @param nStatus The result, 0 on success and 13 when the card was already formatted.
     * @ghidraAddress 0x002d9628
     */
    virtual void OnCardFormatted(int nPortSlot, int nStatus);

    /**
     * Run StartSaveSpaceCheck().
     *
     * MemcardUser slot 13.
     *
     * @param nPortSlot The card the personas came from. The body does not read it.
     * @param nStatus The result. The body does not read it.
     * @ghidraAddress 0x002deb98
     */
    virtual void OnPersonasLoaded(int nPortSlot, int nStatus);

    /**
     * Run StartLoadPersonas().
     *
     * MemcardUser slot 14.
     *
     * @param nPortSlot The card the settings came from. The body does not read it.
     * @param nStatus The result. The body does not read it.
     * @ghidraAddress 0x002deb70
     */
    virtual void OnGlobalSettingsLoaded(int nPortSlot, int nStatus);

    /**
     * Show the detection notice and list the connected cards.
     *
     * Slot 39. Clears mAutosaveNoticeTime and GlobalSettings::mCardSlots, and queues the listing
     * with this screen as the receiver. The title is inferred.
     *
     * @ghidraAddress 0x002d8b80
     */
    virtual void StartDetect();

    /**
     * Show the loading notice and load the personas from the card in port 1.
     *
     * Slot 40. Sets mUnknown98 and empties MetPersonaData::loadList() before the load fills it.
     * The title is inferred.
     *
     * @ghidraAddress 0x002db328
     */
    virtual void StartLoadPersonas();

    /**
     * Respond to finding no usable card in port 1.
     *
     * Slot 41. The body is empty. The title is inferred from the `mem_check` dialogue that
     * MetLocPickCharScreen raises in its override.
     *
     * @ghidraAddress 0x002deaa8
     */
    virtual void OnNoCard() {
    }

    /**
     * Respond to the probe ending.
     *
     * Slot 42. The body is empty. The title is inferred.
     *
     * @ghidraAddress 0x002deab0
     */
    virtual void OnDetectFinished() {
    }

    /**
     * Check the free space on the card in port 1.
     *
     * Slot 43. Queues the check with this screen as the receiver. The title is inferred.
     *
     * @ghidraAddress 0x002debc0
     */
    virtual void StartSaveSpaceCheck();

private:
    int mUnknown90; // +0x90
    int mUnknown94; // +0x94

protected:
    // Set by StartLoadPersonas() and by the MetLocPickCharScreen override at 0x002ba148. +0x98
    int mUnknown98;

private:
    // The renderer time the autosave notice went up, or 0 while it is not showing.
    float mAutosaveNoticeTime;
};
