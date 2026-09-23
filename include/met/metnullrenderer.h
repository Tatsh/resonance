#pragma once

#include "app/rendererbase.h"
#include "game/gameparams.h"

class HxStr;
class Message;
class RawControllerMsg;

/**
 * Front-end renderer that draws nothing, chosen in place of MetRenderer by a configuration option.
 *
 * `15MetNullRenderer` in the RTTI descriptor at `0x008eff00`, with RendererBase as its one public
 * base at offset 0. MetaGameWorld::CreateRenderer() allocates it at 0x88 bytes under the
 * `MsgSink` tag when PythonEvt::QueryOption() reports option 0xcb set. Its vtable at
 * `0x00801d58` has eleven entries, the same length as RendererBase's. Against that table it
 * overrides the destructor and slots 3, 7, and 8, the three RendererBase leaves pure, which is
 * what makes the class concrete. Slots 4 and 5 have two-instruction empty bodies of their own at
 * `0x00311a10` and `0x00311a18`, the base's empty bodies re-emitted rather than overrides, and are
 * not declared.
 *
 * In place of the front-end screens, the X button evaluates a ruleset script template and loads
 * the level it describes, and slot 8 starts a local game once the load finishes.
 */
class MetNullRenderer : public RendererBase {
public:
    /**
     * Construct the renderer and start the front-end loads.
     *
     * Seeds the R250 generator with a fixed value, creates MetFreqMakerAssetManager and starts its
     * asset load, creates GlobalSettings, enables drawing on the game manager, and rebuilds the
     * stage lists.
     *
     * @ghidraAddress 0x0030e2c8
     */
    MetNullRenderer();

    /**
     * Destroy MetFreqMakerAssetManager through MetFreqMakerAssetManager::Destroy().
     *
     * @ghidraAddress 0x0030e400
     */
    virtual ~MetNullRenderer();

    /**
     * Handle one message. MsgSink slot 3, pure in RendererBase.
     *
     * Only a RawControllerMsg is acted on, through OnRawController().
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00311ff0
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Unrecovered. RendererBase slot 7, pure in the base, with an empty body here.
     *
     * @ghidraAddress 0x00311a20
     */
    virtual void OnUnknownSlot7();

    /**
     * Start the game once the ruleset's level has loaded, then pump the timers in the front end.
     *
     * RendererBase slot 8, pure in the base. Both Renderer::PollCommon() and Renderer::PollLevel()
     * run on every call. When a load is pending and both report done, the roster is replaced with
     * one default persona per player, each identified as `freq player` and its index, the game mode
     * becomes solo for one player and local otherwise, the settings are applied, and a
     * BeginGameLocalMsg is queued. MainLoop::PumpTimers() runs whenever there is no game world.
     *
     * @ghidraAddress 0x0030f320
     */
    virtual void OnUnknownSlot8();

private:
    /**
     * Act on a pressed joystick button.
     *
     * The X button in the front end plays `SND_MET_SLIDE` and reads a sequence from script template
     * 0xcc as the level, the ruleset (`jam` or `game`), the difficulty, the player count, and the
     * arena, then starts the common and level loads for slot 8 to finish. In a game the X button
     * queues an UnpauseGameSystemMsg and runs GrooveWorld::PostExitMode2(). Button 4 unloads the
     * level and the common data. An unrecognised ruleset is fatal. The name is inferred.
     *
     * @param pMsg The controller message.
     * @ghidraAddress 0x0030e4c0
     */
    void OnRawController(RawControllerMsg *pMsg);

    /**
     * Turn a ruleset into the play mode GameParams::mUnknown1c records.
     *
     * OnRawController() has this body expanded in place, and the out-of-line copy has no caller.
     * The name is inferred.
     *
     * @param ruleset The ruleset, `jam` or `game`.
     * @return 2 for `jam` and 1 for `game`. Any other text is fatal, after which 0 is returned.
     * @ghidraAddress 0x00311f80
     */
    static int ParseRuleset(const HxStr &ruleset);

    GameParams mUnknown48; // +0x48, the settings the ruleset fills
    int mUnknown80;        // +0x80, set while a ruleset load is pending
    int mUnknown84;        // +0x84, the ruleset's player count
};
