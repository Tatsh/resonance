#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"

class Message;
class Player;

/**
 * One participant's view of a session, driven by messages.
 *
 * `Gamer` in the RTTI descriptor at `0x00901cb0`, over `MsgSink` and `MsgSource`. Two vtables
 * belong to it, one per base, both walked to their terminator: the primary at `0x007ce820` with
 * delta 0, which is the `MsgSink` table because `MsgSink` is the first base, and the `MsgSource`
 * table at `0x007ce7f8` with delta -4.
 *
 * Both are four entries, so the class **adds no virtual of its own**. It overrides only
 * `HandleMessage`, inheriting `MsgSink::Handle` and both `MsgSource` virtuals unchanged.
 *
 * The base subobjects account for `+0x00` through `+0x17`. The destructor at `0x00110930` restores
 * the primary table at `+0x00` and the `MsgSource` one at `+0x14`, which is where `MsgSource`
 * places its own vptr over its `mSinks` vector, so its subobject sits at `+0x04` exactly as the
 * -4 delta states. This class's own members start at `+0x18`.
 *
 * Recovery is partial. Its own members are not enumerated. The destructor releases two owned
 * objects at `+0x90` and `+0x94` by invoking slot 1 of each with the deleting flag set, and calls
 * a helper at `0x00116c40` before doing so. A further member at `+0x8c` is read by the routine at
 * `0x00111fa8`, which `GamerConstructCmd::Execute` calls with this object.
 */
class Gamer : public MsgSink, public MsgSource {
public:
    /** @ghidraAddress 0x00110930 */
    virtual ~Gamer();

    /**
     * Receive one message.
     *
     * Not reconstructed. It dispatches on `Message::Type()` down a chain beginning with the
     * identity at `0x006d0184`, and gates on the members at `+0x2c`, `+0x30`, and `+0x18`.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00112978
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Record a player against one bar of one track.
     *
     * The body is not written. It forwards all three arguments to slot 2 of the owned object at
     * `+0x90`, whose class is unrecovered. TrackData::SetOwner() is the recovered caller.
     *
     * @param nTrack The track's index.
     * @param nBar The bar.
     * @param pPlayer The player.
     * @ghidraAddress 0x00116828
     */
    void SetBarOwner(int nTrack, int nBar, Player *pPlayer);

    /**
     * Ask about one bar of one track.
     *
     * The body is not written. It forwards both arguments to slot 5 of the owned object at `+0x90`
     * and returns its answer. TrackData::QueryBar() is the recovered caller, and Catcher treats a
     * zero answer as a bar that cannot be caught.
     *
     * @param nTrack The track's index.
     * @param nBar The bar.
     * @return The answer of the owned object.
     * @ghidraAddress 0x00116858
     */
    int QueryBar(int nTrack, int nBar);
};
