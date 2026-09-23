#pragma once

#include <vector>

#include "app/msgsink.h"

class Message;
class Player;
class Renderer;
class ScreenAnim;

namespace Rnd {
class Mat;
class Mesh;
} // namespace Rnd

/**
 * Game-side controller of the arena screens around the tunnel.
 *
 * `8TnlArena` in the RTTI descriptor at `0x008ef930`, with MsgSink as its one public base at offset
 * 0. Its type function is at `0x0040c138`. It is a game-side tunnel class rather than a Rnd one.
 *
 * The table at `0x00817390` has four entries, the same length as MsgSink's table at `0x007ccc40`,
 * and the class therefore introduces no virtual. It overrides the destructor at slot 1 and
 * HandleMessage() at slot 3, and inherits MsgSink::Handle() at slot 2.
 *
 * The object is 0x2c bytes, the size Renderer's constructor requests under the MsgSink tag at
 * `0x0042c8fc`. The constructor records the object in g_pTnlArena and the destructor clears it.
 *
 * The arena shows the game on four screens, whose materials are `screen01.mat` to `screen04.mat`.
 * The constructor finds every Rnd::Mesh that uses each of the four and records the mesh with its
 * material in mUnknown10. It then builds one PlayerMaterial per world player, and one of three
 * ScreenAnim classes by mode. Configuration code 0x3a1 selects the plain ScreenAnim, game mode 1
 * selects SoloScreenAnim, and every other mode selects MultiScreenAnim.
 */
class TnlArena : public MsgSink {
public:
    /**
     * One mesh that shows an arena screen, with the material it had when the arena was built.
     *
     * A plain eight-byte record. The name is inferred.
     */
    struct ScreenMesh {
        Rnd::Mesh *mMesh; /*!< The mesh. +0x00 */
        Rnd::Mat *mMat;   /*!< The material the mesh used at construction. +0x04 */
    };

    /**
     * One world player with the `HUD freq%d.mat` material its index selects.
     *
     * A plain eight-byte record allocated with the untagged scalar allocator. The name is inferred.
     */
    struct PlayerMaterial {
        /**
         * Record the player and resolve its material.
         *
         * @param pPlayer The player. Its word at `+0x20` fills the `%d`.
         * @ghidraAddress 0x00406120
         */
        PlayerMaterial(Player *pPlayer);

        Player *mPlayer; /*!< The player. +0x00 */
        Rnd::Mat *mMat;  /*!< The resolved material, or null. +0x04 */
    };

    /**
     * Build the arena screens for the game about to start.
     *
     * The body is not written. It needs declarations of ScreenAnim, of SoloScreenAnim with its
     * constructor at `0x004066d0`, and of MultiScreenAnim with its constructor at `0x00406200`, and
     * it needs the GrooveWorld player vector.
     *
     * @param pRenderer The renderer that constructs this object. The body does not read it.
     * @ghidraAddress 0x004067e8
     */
    TnlArena(Renderer *pRenderer);

    /**
     * Stop the screen animation and restore every screen mesh's original material.
     *
     * Runs slot 3 of the animation with 1, deletes it, frees every PlayerMaterial, and then
     * restores each mesh's material twice over, once through `0x0040c938` and once inline. The body
     * is not written.
     *
     * @ghidraAddress 0x00406de0
     */
    virtual ~TnlArena();

    /**
     * Act on one message the renderer sends on.
     *
     * A PointAmountMsg runs slot 4 of the animation. A JuiceAmountMsg, only in game mode 1 and play
     * mode 1 and only while mUnknown24 is -1, sets the level from the juice amount (2 above 0.85, 0
     * below 0.2, and 1 otherwise) and passes it to slot 3 of the animation. A WinMsg in game mode 1
     * with a non-empty list passes one level higher to slot 3. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00406ff0
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Advance the screen animation to one song position.
     *
     * Forwards to slot 2 of the animation. Renderer::OnUnknownSlot7() is the caller, and the title
     * is inferred from it.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress 0x0040c958
     */
    void SetFrame(float flFrame);

private:
    std::vector<PlayerMaterial *> mUnknown04; // +0x04
    std::vector<ScreenMesh> mUnknown10;       // +0x10
    ScreenAnim *mUnknown1c;                   // +0x1c
    // Globals::GetGameMode() at construction.
    int mUnknown20; // +0x20
    // Starts at -1, and HandleMessage() acts on a JuiceAmountMsg only while it still is.
    int mUnknown24; // +0x24
    // The level last passed to slot 3 of the animation. Starts at 1, or at the play mode when that
    // is 2.
    int mUnknown28; // +0x28
};

/**
 * The arena that exists, or null.
 *
 * @ghidraAddress 0x006dd560
 */
extern TnlArena *g_pTnlArena;
