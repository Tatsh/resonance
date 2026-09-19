#pragma once

#include "met/metscreenmultisoundbank.h"

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
};
