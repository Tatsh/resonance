#pragma once

#include <vector>

#include "app/msgsource.h"
#include "app/rendererbase.h"
#include "msg/barstatusmsg.h"
#include "msg/gamebeginmsg.h"

class AppTunnel;
class GameParams;
class HxStr;
class Message;
class Overlay;
class Player;
class RndAsyncLoader;
class TnlArena;

namespace Rnd {
class View;
} // namespace Rnd

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * In-game renderer that drives the tunnel, the overlay, and the arena for one game.
 *
 * Its RTTI descriptor is at `0x008efc70`. It has two public non-virtual bases, MsgSource at `+0x00`
 * and RendererBase at `+0x14`. Its type function is at `0x00431f18`.
 *
 * The base offsets follow from the base sizes in the same way as for MetRenderer. MsgSource is 0x14
 * bytes and RendererBase is 0x48, and the first member of this class is therefore at `+0x5c`. The
 * object is 0xa8 bytes, the size the allocation at `0x0018cadc` requests under the MsgSink tag.
 * The allocating routine, at `0x0018caa8`, is the constructor's one caller and stores the result at
 * `+0x28` of the world.
 *
 * Two tables belong to the class. The MsgSource table at `0x0081a110` has four entries, the same
 * length as MsgSource's table, and only the destructor differs. The RendererBase table at
 * `0x0081a0b0` has eleven entries and adjusts `this` by `-20`. A diff against RendererBase's table
 * at `0x007d2d20` reads overrides at slots 1, 3, 6, 7, and 8, and inheritance at slots 2, 4, 5, 9,
 * and 10. Slots 3, 7, and 8 are pure in the base, and this class makes the renderer concrete.
 *
 * The constructor records the object in g_pRenderer and the destructor clears it. The three
 * objects the renderer builds (AppTunnel, Overlay, and TnlArena) are also its three message sinks.
 * MsgSource::Send() delivers every message HandleMessage() does not consume to all three.
 *
 * The translation unit also defines seven static routines over five async loaders and two cached
 * names, all file-scope globals. The front end starts the loads and the constructor waits for them.
 *
 * The unreferenced forwarder at `0x00432440` in this unit, byte-identical to
 * MsgJoiner::HandleMessage() at `0x00195b70`, has its unwind record at `0x006e32b8` as its only
 * reference and is recorded here rather than declared.
 */
class Renderer : public MsgSource, public RendererBase {
public:
    /**
     * One cell of the bar grid, a plain record of the last BarStatusMsg state per track and bar.
     *
     * The record is 0x20 bytes. The constructor fills every cell with the static NullPlayer at
     * `0x0066f930`, 0, -1, an empty mask, and -1 in field order. The name is inferred from the grid
     * indexing in GetCell().
     */
    struct Cell {
        Player *mPlayer; /*!< The player a BarStatusMsg last reported. +0x00 */
        int mEnabled;    /*!< The enabled field a BarStatusMsg last reported. +0x04 */
        int mPowerup;    /*!< The powerup a BarStatusMsg last reported, or -1. +0x08 */
        BarStatusMsg::Effects mEffects; /*!< The effect mask a BarStatusMsg last reported. +0x10 */
        int mBar;                       /*!< Bar the cell last recorded, or -1. +0x18 */
    };

    /**
     * Build the tunnel, overlay, and arena for the game about to start.
     *
     * Pumps the timers and polls every async load until the three common loaders and then the
     * level and arena loaders report done, the second wait against the arena and level names of
     * configuration codes 0x27c and 0x278. Each wait ends with one more poll whose result is
     * discarded. Resolves `outer.env`, whose fog colour becomes the device clear colour after the
     * VRAM table is cleared, and the views `outer.view`, `tnl.view`, `hud.view`, and one
     * `tnl local%d.view` per local player, numbered from 1. Reads configuration codes 0x397 and
     * 0x3a2 into the two debug-overlay switches, builds the three sinks, sizes the cell grid from
     * the `tunnel` object, and records itself in g_pRenderer.
     *
     * @ghidraAddress NTSC-U/C: 0x0042c2e0
     * @ghidraAddress PAL: 0x00467da0
     */
    Renderer();

    /**
     * Release the three sinks and clear g_pRenderer.
     *
     * Slot 1 of both tables. The sinks are deleted in reverse order of construction.
     *
     * @ghidraAddress NTSC-U/C: 0x0042ce08
     * @ghidraAddress PAL: 0x004689c0
     */
    virtual ~Renderer();

    /**
     * Dispatch one message.
     *
     * RendererBase slot 3, pure in the base. A GameBeginMsg runs OnMsg(), a BarStatusMsg runs
     * OnBarStatus(), and a PointAmountMsg runs OnPointAmount(). Every other message is sent on to
     * the sinks.
     *
     * @param pMsg The message to dispatch.
     * @ghidraAddress NTSC-U/C: 0x0042d3b8
     * @ghidraAddress PAL: 0x00468f70
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Sample the song clock, then deliver every queued message.
     *
     * RendererBase slot 6. The tick is stored as a float in mSongTick before
     * RendererBase::PollMessages() drains the queue.
     *
     * @ghidraAddress NTSC-U/C: 0x00432460
     * @ghidraAddress PAL: 0x0046e138
     */
    virtual void PollMessages();

    /**
     * Advance everything the renderer draws to the sampled song tick.
     *
     * RendererBase slot 7, pure in the base. The tick goes to the three sinks and the three views,
     * and each view then recomposes its world transform.
     *
     * @ghidraAddress NTSC-U/C: 0x004324a0
     * @ghidraAddress PAL: 0x0046e178
     */
    virtual void Update();

    /**
     * Draw the frame.
     *
     * RendererBase slot 8, pure in the base. Draws `outer.view`, sets up an alternative GS draw
     * context while g_nLsdMode is set, draws each local tunnel view after
     * AppTunnel::PrepareLocalView() has placed it, draws `hud.view` and the overlay, and finally
     * the two debug overlays under mDrawTimingGraph and mDrawRenderStats.
     *
     * @ghidraAddress NTSC-U/C: 0x0042d258
     * @ghidraAddress PAL: 0x00468e10
     */
    virtual void Draw();

    /**
     * Look up the cell for one track and bar.
     *
     * The bar wraps modulo the slice count, with a negative remainder raised into range. The title
     * is inferred.
     *
     * @param nTrack The track. It selects the row.
     * @param nBar The bar. It selects the column.
     * @return The cell.
     * @ghidraAddress NTSC-U/C: 0x004322b8
     * @ghidraAddress PAL: 0x0046df90
     */
    Cell *GetCell(int nTrack, int nBar);

    /**
     * Start the three common loads unless they have already started.
     *
     * Rewinds the zone `rndCommon`, starts `tunnel/tunnel_new.rnd`, `tunnel/launch.rnd`, and
     * `hud/hud.rnd`, passing the zone's index as each loader's third argument, and enqueues all
     * three. The European release also starts `hud/game_fonts<suffix>.rnd`, the suffix being
     * GetFontLanguageSuffix(), and enqueues that load first.
     *
     * @ghidraAddress NTSC-U/C: 0x0042b7a0
     * @ghidraAddress PAL: 0x00466d60
     */
    static void LoadCommon();

    /**
     * Release the level loads and then the three common loads.
     *
     * Each common loader is unloaded and then deleted, `hud/hud.rnd` first, and its global is
     * cleared. The European release unloads its font load after `hud/hud.rnd`.
     *
     * @ghidraAddress NTSC-U/C: 0x0042bb38
     * @ghidraAddress PAL: 0x004673e8
     */
    static void UnloadCommon();

    /**
     * Start the level and arena loads for one game unless they are already the ones loaded.
     *
     * A changed level name drops both loaders, and a changed arena name drops the arena loader.
     * Each missing loader then rewinds its zone (`rndTnlLevel` or `rndTnlArena`) and starts
     * `levels/<level>/images/images.rnd` or `arenas/<arena>/<arena>.rnd`, and the new name is
     * recorded.
     *
     * @param params The settings whose level and arena names are loaded.
     * @ghidraAddress NTSC-U/C: 0x0042bbe8
     * @ghidraAddress PAL: 0x004674c8
     */
    static void LoadLevel(const GameParams &params);

    /**
     * Delete the level and arena loaders and forget both names.
     *
     * @ghidraAddress NTSC-U/C: 0x00432128
     * @ghidraAddress PAL: 0x0046de00
     */
    static void UnloadLevel();

    /**
     * Poll the common loads, three or, in the European release, four.
     *
     * @param pflProgress Receives the mean progress of the loads. Not written when the loads have
     *        not started.
     * @return 1 when all are done, and 0 otherwise.
     * @ghidraAddress NTSC-U/C: 0x00432080
     * @ghidraAddress PAL: 0x0046dd38
     */
    static int PollCommon(float *pflProgress);

    /**
     * Poll the arena and level loads.
     *
     * @param pflProgress Receives the mean progress of the two loads. Not written when the arena
     *        load has not started.
     * @return 1 when both are done, and 0 otherwise.
     * @ghidraAddress NTSC-U/C: 0x004321a8
     * @ghidraAddress PAL: 0x0046de80
     */
    static int PollLevel(float *pflProgress);

    /**
     * Report whether one arena and level are loaded.
     *
     * The image has no caller.
     *
     * @param arena The arena name to compare.
     * @param level The level name to compare.
     * @return 1 when both names match the recorded ones and both loads are done, and 0 otherwise.
     * @ghidraAddress NTSC-U/C: 0x00432228
     * @ghidraAddress PAL: 0x0046df00
     */
    static int IsLevelLoaded(const HxStr &arena, const HxStr &level);

private:
    // NTSC-U/C: 0x004322f8, PAL: 0x0046dfd0
    // Runs script template 1000. HandleMessage() inlines the body, and this copy has
    // no caller.
    void OnMsg(const GameBeginMsg &msg);

    // NTSC-U/C: 0x0042d068, PAL: 0x00468c20
    // Merges the payload into the cell for its track and bar, then tells the overlay
    // when the bar or the effect mask changed and the tunnel when the bar or any of the other
    // three fields changed.
    void OnBarStatus(BarStatusMsg *pMsg);

    // NTSC-U/C: 0x00432318, PAL: 0x0046dff0
    // Sends the message on, then outside game mode 1 and play mode 2 finds the one
    // world player with the highest score and reports a change of leader to the tunnel and the
    // overlay. A tie for the top score produces no leader. HandleMessage() inlines the body, and
    // this copy has no caller.
    void OnPointAmount(Message *pMsg);

    // The world's song clock, read once by the constructor through Globals::GetSongClock().
    Sch::TickClock *mSongClock; // +0x5c

public:
    /**
     * The song tick PollMessages() last sampled, in MIDI ticks. +0x60
     *
     * Public because Overlay's handlers at `0x0041fdd8` and `0x0042b068` and
     * AppTunnel::OnBarChanged() read it directly through the renderer they record, and the image
     * has no accessor for it.
     */
    float mSongTick;

private:
    // Draw the subsystem timing graph. Filled from configuration code 0x397.
    int mDrawTimingGraph; // +0x64
    // Draw the render-statistics overlay. Filled from configuration code 0x3a2.
    int mDrawRenderStats; // +0x68
    // The bar grid, mRowCount rows of mCellsPerRow cells.
    std::vector<Cell> mCells; // +0x6c
    // Cells per row, the `tunnel` object's slice count.
    int mCellsPerRow; // +0x78
    // Row count, the `tunnel` object's ring count.
    int mRowCount; // +0x7c
    // `outer.view`.
    Rnd::View *mOuterView; // +0x80
    // `tnl.view`.
    Rnd::View *mTunnelView; // +0x84
    // `hud.view`.
    Rnd::View *mHudView; // +0x88
    // One `tnl local%d.view` per local player.
    std::vector<Rnd::View *> mLocalViews; // +0x8c
    AppTunnel *mTunnel;                   // +0x98
    Overlay *mOverlay;                    // +0x9c
    TnlArena *mArena;                     // +0xa0
    // The player OnPointAmount() last reported as the leader, or null.
    Player *mLeader; // +0xa4
};

/**
 * The renderer that exists, or null.
 *
 * The constructor stores the object and the destructor clears the word. The body of the script
 * function `lsdmode` at `0x0042d558` reads it before toggling g_nLsdMode.
 *
 * @ghidraAddress NTSC-U/C: 0x006e253c
 * @ghidraAddress PAL: 0x00725e44
 */
extern Renderer *g_pRenderer;

/**
 * Switch that selects the alternative GS draw context in Renderer::Draw().
 *
 * The script function `lsdmode` inverts it, and only while a renderer exists. The translation
 * unit's static initialiser at `0x00431ab8` registers that function. The global's name follows the
 * script name.
 *
 * @ghidraAddress NTSC-U/C: 0x006e2538
 * @ghidraAddress PAL: 0x00725e40
 */
extern int g_nLsdMode;

/**
 * Loader for `tunnel/tunnel_new.rnd`.
 *
 * @ghidraAddress NTSC-U/C: 0x006e2510
 * @ghidraAddress PAL: 0x00725e18
 */
extern RndAsyncLoader *g_pTunnelLoader;

/**
 * Loader for `tunnel/launch.rnd`.
 *
 * @ghidraAddress NTSC-U/C: 0x006e2514
 * @ghidraAddress PAL: 0x00725e1c
 */
extern RndAsyncLoader *g_pLaunchLoader;

/**
 * Loader for `hud/hud.rnd`.
 *
 * @ghidraAddress NTSC-U/C: 0x006e2518
 * @ghidraAddress PAL: 0x00725e20
 */
extern RndAsyncLoader *g_pHudLoader;

#ifdef VIDEO_STANDARD_PAL
/**
 * Loader for `hud/game_fonts<suffix>.rnd`, the fonts of the language GetLanguage() reports.
 *
 * The European release added the loader. The name is inferred.
 *
 * @ghidraAddress PAL: 0x00725e24
 */
extern RndAsyncLoader *g_pGameFontsLoader;
#endif

/**
 * Loader for the arena Renderer::LoadLevel() last started.
 *
 * @ghidraAddress NTSC-U/C: 0x006e251c
 * @ghidraAddress PAL: 0x00725e28
 */
extern RndAsyncLoader *g_pArenaLoader;

/**
 * Loader for the level Renderer::LoadLevel() last started.
 *
 * @ghidraAddress NTSC-U/C: 0x006e2520
 * @ghidraAddress PAL: 0x00725e2c
 */
extern RndAsyncLoader *g_pLevelLoader;

/**
 * Arena name g_pArenaLoader was started for, empty when none.
 *
 * @ghidraAddress NTSC-U/C: 0x006e2528
 * @ghidraAddress PAL: 0x00725e30
 */
extern HxStr g_arenaName;

/**
 * Level name g_pLevelLoader was started for, empty when none.
 *
 * @ghidraAddress NTSC-U/C: 0x006e2530
 * @ghidraAddress PAL: 0x00725e38
 */
extern HxStr g_levelName;
