#pragma once

#include <vector>

#include "met/metscreenmultisoundbank.h"
#include "os/hxstr.h"

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
 * strings, and sets mUnknownd4 and mOwnerPad to -1 and mUnknownd0 and mShowing to zero.
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
     * Hangs `dlg_<n>but.view` under mUnknown98, writes the title and the text, sizes the frame by
     * hanging `dlg_small.view`, `dlg_medium.view`, or `dlg_large.view` under `dlg_group.view` for
     * fewer than three, fewer than six, or more text lines, labels the buttons of the list the
     * count selects and selects the first, and sets mUnknownd4 to -1. The title is inferred.
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

private:
    MetButtonList *mUnknown8c; // +0x8c, the one-button list
    MetButtonList *mUnknown90; // +0x90, the two-button list
    MetButtonList *mUnknown94; // +0x94, whichever list Refresh() selected, or null
    Rnd::View *mUnknown98;     // +0x98, the view each button layout hangs under
    Rnd::Text *mUnknown9c;     // +0x9c, the text object the message is drawn into
    Rnd::Text *mUnknowna0;     // +0xa0, the text object the title is drawn into
    // The button labels Refresh() writes. +0xa4
    std::vector<HxStr> mButtons;
    // The dialogue name reported back to the owner. +0xb0
    HxStr mName;
    // The screen the dialogue reports to. +0xb8
    MetScreen *mOwner;
    // The number of buttons, 1 or 2, which picks the layout and the button list. +0xbc
    int mButtonCount;
    HxStr mText;    // +0xc0
    HxStr mTitle;   // +0xc8
    int mUnknownd0; // +0xd0
    // Refresh() resets it to -1. +0xd4
    int mUnknownd4;
    // Non-zero while the dialogue is showing, which is when Show() refreshes it in place. +0xd8
    int mShowing;
    // The controller SetOwnerPad() records, or -1 after Show(). +0xdc
    int mOwnerPad;
};
