#pragma once

#include <vector>

#include "met/metscreenmultisoundbank.h"
#include "os/hxstr.h"

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
 * container. It writes `+0x8c`, `+0x90`, a vector at `+0xa4`, then `+0xb0`, `+0xb4`, the run from
 * `+0xc0` to `+0xd8`, and `+0xdc`.
 *
 * It and MetLogoScreen are the only two classes that override slot 3, MsgSink::HandleMessage, with
 * a body.
 *
 * The object is at least 0xe0 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x002ec450`.
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
     * @ghidraAddress 0x002ec450
     */
    virtual ~MetMsgScreen();

    /**
     * Fill the registered message screen and bring it up.
     *
     * The screen registered as `MetMsgScreen` receives the dialogue name at `+0xb0`, the title at
     * `+0xc8`, the text at `+0xc0`, nUnknown at `+0xbc`, and the owner at `+0xb8`, and `+0xdc`
     * becomes -1. A non-empty button list is installed through `0x002ec5d8`. A screen already
     * showing, flagged at `+0xd8`, is refreshed through `0x002ecd10`. Otherwise MetSonyScreen
     * pushes the message screen and makes it the active panel. The title is inferred. The body is
     * not written yet.
     *
     * @param name The dialogue name, which the owner's OnMsgScreenShown() and
     * OnMsgScreenDismissed() receive back.
     * @param title The title line.
     * @param text The message text.
     * @param nUnknown Stored at `+0xbc`. Its meaning is not recovered.
     * @param buttons The button labels, or an empty list for the default buttons.
     * @param pOwner The screen the dialogue reports to.
     * @ghidraAddress 0x002ebc98
     */
    static void Show(const HxStr &name,
                     const HxStr &title,
                     const HxStr &text,
                     int nUnknown,
                     const std::vector<HxStr> &buttons,
                     MetScreen *pOwner);

    /**
     * Fill the registered message screen and make it the active panel.
     *
     * The same as Show(), except that a screen already showing is also made the active panel again
     * after the refresh. The title is inferred. The body is not written yet.
     *
     * @param name The dialogue name.
     * @param title The title line.
     * @param text The message text.
     * @param nUnknown Stored at `+0xbc`. Its meaning is not recovered.
     * @param buttons The button labels, or an empty list for the default buttons.
     * @param pOwner The screen the dialogue reports to.
     * @ghidraAddress 0x002ebf40
     */
    static void ShowActive(const HxStr &name,
                           const HxStr &title,
                           const HxStr &text,
                           int nUnknown,
                           const std::vector<HxStr> &buttons,
                           MetScreen *pOwner);

    /**
     * Limit the registered message screen to one controller.
     *
     * Writes `+0xdc` of the screen registered as `MetMsgScreen`, which Show() and ShowActive()
     * reset to -1. Every caller passes MetSaveRemix::mUnknownc8, the index of the controller that
     * owns the save. The result of the cast is not checked. The title is inferred. The body is not
     * written yet.
     *
     * @param nPad The controller index.
     * @ghidraAddress 0x002f0348
     */
    static void SetOwnerPad(int nPad);
};
