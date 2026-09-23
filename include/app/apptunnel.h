#pragma once

#include "app/msgsink.h"

class Message;
class Player;
class Renderer;

/**
 * Game-side driver of the tunnel the in-game renderer draws.
 *
 * `9AppTunnel` in the RTTI descriptor at `0x008f0860`, with MsgSink as its one public base at
 * offset 0. Its type function is at `0x00453ff8`. The class sits in `app/` beside Renderer, its one
 * constructor caller, and it is a game-side class rather than a Rnd one.
 *
 * The table at `0x0081ba38` has four entries, the same length as MsgSink's table at `0x007ccc40`,
 * and the class therefore introduces no virtual. It overrides the destructor at slot 1 and
 * HandleMessage() at slot 3, and inherits MsgSink::Handle() at slot 2.
 *
 * The object is 0x170 bytes, the size Renderer's constructor requests under the MsgSink tag at
 * `0x0042c89c`. The constructor zeroes eight three-word vectors at `+0x24` through `+0x78`, one at
 * `+0x8c`, a one-word `std::list` at `+0xa8` built under the tag `stl_list`, and two more vectors
 * at `+0xac` and `+0xb8`. The destructor also releases vectors at `+0xd4`, `+0xe0`, `+0xec`,
 * `+0xf8`, and `+0x104`, and tears down the objects at `+0x84`, `+0x88`, and `+0xc4` through
 * `0x00412490`, `0x00433320`, and `0x00455ef0`, and those at `+0x18` and `+0x1c` through
 * `0x00457010` and `0x0043f920`. None of those classes is recovered yet, and the members past
 * `+0x0c` are therefore recorded as one reserved span.
 */
class AppTunnel : public MsgSink {
public:
    /**
     * Build the tunnel for the game about to start.
     *
     * Records the renderer, the game mode, and the play mode first. The body is not written.
     *
     * @param pRenderer The renderer that constructs this object.
     * @ghidraAddress 0x00442020
     */
    AppTunnel(Renderer *pRenderer);

    /**
     * Release every tunnel object.
     *
     * Clears g_pAppTunnel first. The body is not written.
     *
     * @ghidraAddress 0x00445740
     */
    virtual ~AppTunnel();

    /**
     * Act on one message the renderer sends on. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00449688
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Advance the tunnel to one song position.
     *
     * Renderer::OnUnknownSlot7() is the caller. The title is inferred from that caller, which hands
     * the same value to Rnd::Animatable::SetFrame() on its three views. The body is not written.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress 0x00446960
     */
    void SetFrame(float flFrame);

    /**
     * Move the leader marker from one player's tunnel section to another's.
     *
     * Does nothing in game mode 1. A null player on either side is skipped. The title is
     * inferred. The body is not written.
     *
     * @param pOldLeader The previous leader, or null.
     * @param pNewLeader The new leader, or null.
     * @ghidraAddress 0x00446838
     */
    void OnLeaderChanged(Player *pOldLeader, Player *pNewLeader);

    /**
     * Update the tunnel for one bar whose state a BarStatusMsg changed.
     *
     * The last three arguments are the cell's three words in the order the call passes them. The
     * title is inferred. The body is not written.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @param nUnknown The word at `+0x14` of the BarStatusMsg.
     * @param pPlayer The cell's player.
     * @param nUnknown08 The cell's `mUnknown08`.
     * @param nUnknown04 The cell's `mUnknown04`.
     * @ghidraAddress 0x004465a0
     */
    void OnBarChanged(
        int nTrack, int nBar, int nUnknown, Player *pPlayer, int nUnknown08, int nUnknown04);

    /**
     * Rotate the tunnel for one local view before the renderer draws it.
     *
     * The rotation is the view index times -45 degrees. Renderer::OnUnknownSlot8() is the caller.
     * The title is inferred. The body is not written.
     *
     * @param nView The index of the local view.
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress 0x00457bd0
     */
    void PrepareLocalView(int nView, float flFrame);

private:
    Renderer *mUnknown04; // +0x04
    // Globals::GetGameMode() at construction.
    int mUnknown08; // +0x08
    // Globals::GetPlayMode() at construction.
    int mUnknown0c; // +0x0c
    // Members from +0x10 to the end of the object, not yet recovered.
    unsigned char mReserved10[0x160]; // +0x10
};

/**
 * The tunnel that exists, or null.
 *
 * The constructor stores the object and the destructor clears the word.
 *
 * @ghidraAddress 0x006e42a0
 */
extern AppTunnel *g_pAppTunnel;
