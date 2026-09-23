#pragma once

#include <vector>

#include "met/metscreenmultisoundbank.h"
#include "os/hxstr.h"

class Message;
class MetButtonList;

namespace Rnd {
class Text;
class View;
} // namespace Rnd

/**
 * Dialogue that shows one message.
 *
 * `12MetMsgScreen` in the RTTI descriptor at `0x008f0100`, with MetScreenMultiSoundBank as its one
 * public non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007fe668`, the same length as the MetScreenMultiSoundBank
 * table, so the class declares no virtual of its own.
 *
 * The constructor at `0x002ec290` takes only the renderer and the load priority, and supplies
 * `dlg` for the screen name, `metagame/Shared` for the directory, and `dialogue` for the
 * container. It zeroes the two button lists, default-constructs the label vector and the three
 * strings, and sets mChoice and mOwnerPad to -1 and mExitTime and mShowing to zero.
 *
 * It and MetLogoScreen are the only two classes that override slot 3, MsgSink::HandleMessage, with
 * a body.
 *
 * The object is 0xe0 bytes, which the factory at `0x002f02c0` fixes by requesting exactly that
 * many with the tag `MsgSink`.
 *
 * Apart from the type function and the destructor, the slots that differ from the
 * MetScreenMultiSoundBank table are 3 `0x002f0640`, 5 `0x002f04d0`, 9 `0x002f0588`, 19
 * `0x002ecba0`, 20 `0x002f0410`, 21 `0x002f02b8`, 22 `0x002f02b0`, 23 `0x002f0450`, 24
 * `0x002f0490`, 26 `0x002f0540`, 33 `0x002f0500`, 36 `0x002f05d0`, 38 `0x002ec6f8`.
 */
class MetMsgScreen : public MetScreenMultiSoundBank {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002ec290
     */
    MetMsgScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the two-button list and then the one-button list.
     *
     * @ghidraAddress 0x002ec450
     */
    virtual ~MetMsgScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002f02c0
     */
    static MetMsgScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Fill the registered message screen and bring it up.
     *
     * The screen registered as `MetMsgScreen` receives the dialogue name, the title, the text, the
     * button count, and the owner, and mOwnerPad becomes -1. A non-empty button list replaces the
     * labels through SetButtons(). A screen already showing is refreshed in place. Otherwise
     * MetSonyScreen pushes the message screen and makes it the active panel. Neither screen lookup
     * is checked for null. The title is inferred.
     *
     * @param name The dialogue name, which the owner's OnMsgScreenShown() and
     * OnMsgScreenDismissed() receive back.
     * @param title The title line.
     * @param text The message text.
     * @param nButtons The number of buttons, which picks the one-button or two-button layout.
     * @param buttons The button labels, or an empty list to retain the current labels.
     * @param pOwner The screen the dialogue reports to.
     * @ghidraAddress 0x002ebc98
     */
    static void Show(const HxStr &name,
                     const HxStr &title,
                     const HxStr &text,
                     int nButtons,
                     const std::vector<HxStr> &buttons,
                     MetScreen *pOwner);

    /**
     * Fill the registered message screen and make it the active panel.
     *
     * The same as Show(), except that a screen already showing is also made the active panel again
     * after the refresh. The title is inferred.
     *
     * @param name The dialogue name.
     * @param title The title line.
     * @param text The message text.
     * @param nButtons The number of buttons.
     * @param buttons The button labels, or an empty list to retain the current labels.
     * @param pOwner The screen the dialogue reports to.
     * @ghidraAddress 0x002ebf40
     */
    static void ShowActive(const HxStr &name,
                           const HxStr &title,
                           const HxStr &text,
                           int nButtons,
                           const std::vector<HxStr> &buttons,
                           MetScreen *pOwner);

    /**
     * Limit the registered message screen to one controller.
     *
     * Writes mOwnerPad of the screen registered as `MetMsgScreen`, which Show() and ShowActive()
     * reset to -1. Every caller passes MetSaveRemix::mUnknownc8, the index of the controller that
     * owns the save. The result of the cast is not checked. The title is inferred.
     *
     * @param nPad The controller index.
     * @ghidraAddress 0x002f0348
     */
    static void SetOwnerPad(int nPad);

    /**
     * Replace the button labels.
     *
     * An empty list does nothing. The title is inferred.
     *
     * @param buttons The new labels.
     * @ghidraAddress 0x002ec5d8
     */
    void SetButtons(const std::vector<HxStr> &buttons);

    /**
     * Rebuild the dialogue for the current name, title, text, and button count.
     *
     * Hangs `dlg_<n>but.view` under mButtonView, writes the title and the text, sizes the frame by
     * hanging `dlg_small.view`, `dlg_medium.view`, or `dlg_large.view` under `dlg_group.view` for
     * fewer than three, fewer than six, or more text lines, labels the buttons of the list the
     * count selects and selects the first, and sets mChoice to -1. The title is inferred.
     *
     * @ghidraAddress 0x002ecd10
     */
    void Refresh();

    /**
     * Assign the dialogue name.
     *
     * Inline. Show() and ShowActive() expand it, and `0x002f0230` is its uncalled out-of-line copy.
     *
     * @param name The new name.
     */
    void SetName(const HxStr &name) {
        mName = name;
    }

    /**
     * Assign the title line.
     *
     * Inline. `0x002f0250` is its uncalled out-of-line copy.
     *
     * @param title The new title.
     */
    void SetTitle(const HxStr &title) {
        mTitle = title;
    }

    /**
     * Assign the message text.
     *
     * Inline. `0x002f0270` is its uncalled out-of-line copy.
     *
     * @param text The new text.
     */
    void SetText(const HxStr &text) {
        mText = text;
    }

    /**
     * Discard the message's type and forward the message to the owner.
     *
     * Slot 3. The type is read through Message::Type() and not used.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x002f0640
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Clear the showing flag, refresh the dialogue, and show the screen. Slot 5.
     *
     * @ghidraAddress 0x002f04d0
     */
    virtual void EnterAndShow();

    /**
     * Start the exit, or schedule it for a dialogue with no buttons. Slot 9.
     *
     * A dialogue with no buttons records an exit time 480 units after the renderer's current time,
     * which slot 26 then waits for. Any other dialogue exits at once through
     * MetScreen::BeginExit().
     *
     * @ghidraAddress 0x002f0588
     */
    virtual void BeginExit();

    /**
     * Act on one navigation command. Slot 19.
     *
     * A dialogue with no buttons ignores every command, and so does a command from a controller
     * other than mOwnerPad when that is not -1. Left and right move the selection through the
     * button list's slots 2 and 3, and select records the selected index as mChoice, clears the
     * active panel, and exits.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x002ecba0
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the slide sound when the dialogue has buttons and accepts nSelector. Slot 20.
     *
     * @param nSelector The controller index the sound is for.
     * @ghidraAddress 0x002f0410
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Silence the leave sound. Slot 21, a two-instruction stub.
     *
     * @ghidraAddress 0x002f02b8
     */
    virtual void PlayLeaveSound(int) {
    }

    /**
     * Silence the high sound. Slot 22, a two-instruction stub.
     *
     * @ghidraAddress 0x002f02b0
     */
    virtual void PlayHighSound(int) {
    }

    /**
     * Play the cycle-left sound when the dialogue has two buttons and accepts nSelector. Slot 23.
     *
     * @param nSelector The controller index the sound is for.
     * @ghidraAddress 0x002f0450
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play the cycle-right sound when the dialogue has two buttons and accepts nSelector. Slot 24.
     *
     * @param nSelector The controller index the sound is for.
     * @ghidraAddress 0x002f0490
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Exit once a scheduled exit time has passed. Slot 26.
     *
     * A zero mExitTime does nothing. Otherwise a time past it clears mExitTime and exits through
     * MetScreen::BeginExit() directly.
     *
     * @param flTime The renderer's current time.
     * @ghidraAddress 0x002f0540
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Mark the dialogue as showing and tell the owner. Slot 33.
     *
     * @ghidraAddress 0x002f0500
     */
    virtual void OnUnknownSlot33();

    /**
     * Mark the dialogue as gone and report the choice to the owner. Slot 36.
     *
     * The owner's OnMsgScreenDismissed() receives mName and mChoice.
     *
     * @ghidraAddress 0x002f05d0
     */
    virtual void OnUnknownSlot36();

    /**
     * Build the button lists and resolve the dialogue's text objects and button view. Slot 38.
     *
     * The two-button list holds `dlg2_01.but` and `dlg2_02.but`, and the one-button list holds
     * `dlg1_01.but`, each with an empty label. `dlg_warning.txt` receives the title,
     * `dlg_message.txt` the message, and `dlg_buts.view` the button layout.
     *
     * @ghidraAddress 0x002ec6f8
     */
    virtual void ResolveContainerViews();

private:
    // 0x002f0610. Inline, and HandleMessage() expands it. The address is its uncalled out-of-line
    // copy.
    void ForwardToOwner(Message *pMsg) {
        mOwner->Handle(pMsg);
    }

    MetButtonList *mOneButtonList; // +0x8c
    MetButtonList *mTwoButtonList; // +0x90
    // Whichever list Refresh() selected by mButtonCount, or null. +0x94
    MetButtonList *mButtonList;
    Rnd::View *mButtonView;  // +0x98, `dlg_buts.view`
    Rnd::Text *mMessageText; // +0x9c, `dlg_message.txt`
    Rnd::Text *mTitleText;   // +0xa0, `dlg_warning.txt`
    // The button labels Refresh() writes. +0xa4
    std::vector<HxStr> mButtons;
    // The dialogue name reported back to the owner. +0xb0
    HxStr mName;
    // The screen the dialogue reports to. +0xb8
    MetScreen *mOwner;
    // The number of buttons, 0, 1, or 2, which picks the layout and the button list. +0xbc
    int mButtonCount;
    HxStr mText;  // +0xc0
    HxStr mTitle; // +0xc8
    // The renderer time slot 26 exits at, or zero for none. +0xd0
    float mExitTime;
    // The button index selected when the dialogue was answered, or -1. +0xd4
    int mChoice;
    // Non-zero while the dialogue is showing, which is when Show() refreshes it in place. +0xd8
    int mShowing;
    // The controller SetOwnerPad() records, or -1 after Show(). +0xdc
    int mOwnerPad;
};
