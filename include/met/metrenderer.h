#pragma once

#include <vector>

#include "app/msgsource.h"
#include "app/rendererbase.h"
#include "met/fadeuser.h"
#include "rnd/view.h"

// MetScreen stores its renderer and the renderer stores its screens, so one of the two
// declarations has to be incomplete. MemcardOp declares MemcardCBHandler the same way.
class MetScreen;
class RawControllerMsg;
class RndAsyncLoader;
class MetCommandMap;
class MetCommandRepeater;
class MetFade;

/**
 * Front-end renderer that owns the screen stack and drives every MetScreen.
 *
 * `11MetRenderer` in the RTTI descriptor at `0x008eef98`, with three public non-virtual bases at
 * fixed offsets, MsgSource at `+0x00`, RendererBase at `+0x14`, and FadeUser at `+0x5c`. Its
 * GetTypeInfo is at `0x00370f98`.
 *
 * MetScreen stores its renderer at `+0x10` and registers itself on it as a message sink through
 * MsgSource::AddSink() during construction, which is how the pointer is known to address the
 * MsgSource subobject at offset 0.
 *
 * All three bases are declared and the three RTTI offsets follow from their sizes rather than being
 * asserted. MsgSource is 0x14 bytes, which places RendererBase at 20. The constructor at
 * `0x00369fb0` confirms that independently: it writes the MsgSource table at `+0x10` rather than at
 * `+0x00`, which is where a class with no base of its own stores its vptr, and the destructor
 * deallocates a three-word `std::vector` at `+0x00` through `+0x0c` below it. RendererBase is 0x48
 * bytes, being its 4-byte MsgSink base, the 0x3c-byte MsgQueue at `+0x04`, and the 8-byte Router at
 * `+0x40`, which places FadeUser at 92. FadeUser is 4 bytes of vptr, which ends the base region at
 * `+0x60`. Each figure is recovered independently of the others, so the three offsets agreeing with
 * the descriptor is a check rather than an assumption.
 *
 * Three vtables belong to the class, one per base, and a slot index restarts in each.
 *
 * The primary table at `0x008091f8` is the MsgSource table and has four entries, the same length as
 * MsgSource's own table at `0x008299f0`. The class therefore declares no virtual of its own, and
 * the only entry that differs is slot 1, the destructor.
 *
 * The RendererBase table at `0x00809198` has eleven entries and adjusts `this` by `-20`. A diff
 * against RendererBase's own table at `0x007d2d20` reads overrides at slots 1, 3, 4, 5, 7, 8, 9,
 * and 10, with slots 2 and 6 inherited. Slots 3, 7, and 8 store the `__pure_virtual` stub at
 * `0x005381a8` in the base, so this class is what makes the renderer concrete.
 *
 * The FadeUser table at `0x00809170` has four entries and adjusts `this` by `-92`. Every entry
 * differs from FadeUser's own table at `0x007ec070`, and the two that matter are slots 2 and 3,
 * both of which are pure in the base.
 *
 * The object is at least 0xd8 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * Six of the eight RendererBase overrides have no recovered name, because RendererBase itself
 * supplies none. Each is documented under the placeholder the base declares it as, and what the
 * override does is recorded on the declaration.
 *
 * Two routines in the class's translation unit are not declared here. `0x00390088` and
 * `0x00390090` are empty bodies that take the renderer and are re-emitted into the translation
 * unit of every screen that runs them, and neither has a recovered name.
 *
 * The bodies listed below are understood and not written, because each needs a routine that no
 * header in this tree declares yet. The blocking dependency of each is recorded below so that the
 * body can be written once the owning subsystem declares it.
 *
 * - The constructor and the destructor both need the asynchronous-request release at `0x003f8240`
 *   and the four no-argument shutdown helpers at `0x00217fa0`, `0x0018ba48`, `0x00383700`, and
 *   MetFreqMakerAssetManager::Destroy() at `0x002551b8`, reached through its out-of-line forwarder
 *   at `0x00254970`.
 * - HandleMessage() needs RawControllerMsg to declare its four-word payload, which
 *   msg/rawcontrollermsg.h records as recovered from Clone() and does not declare. The payload
 *   record is declared as MetControllerReading in msg/metcontrollerreading.h until it does.
 * - OnUnknownSlot5() needs the named-sound player at `0x0012eba0`, which is not the one
 *   app/playsound.h declares, and the singleton getter at `0x00217f30`.
 * - OnUnknownSlot7() needs the two asynchronous predicates at `0x00464628` and `0x00460b20`, the
 *   device clear-colour setter at `0x0049b368`, and `0x00381ef8`.
 * - OnUnknownSlot8() and OnUnknownSlot10() both need
 *   GfxDevice::DrawSubsystemTimingGraph() at `0x0049c778` and
 *   GfxDevice::DrawRenderStatsOverlay() at `0x0049bec8`, and OnUnknownSlot8() also needs
 *   `0x001716d0`.
 * - RemoveScreen() needs Rnd::Transformable::RemoveTrans() at `0x004f09c0` and
 *   Rnd::Drawable::RemoveDraw() at `0x00503360`, and it reads MetScreen::mUnknown14, which
 *   metscreen.h declares private.
 * - ResolveSceneViews() needs the fade constructor's sibling at `0x00384300`, and
 *   ResolveArenaView() needs AddBackgroundView().
 */
class MetRenderer : public MsgSource, public RendererBase, public FadeUser {
    // MetaGameWorld::OnUnknownQuery003d48c0() at 0x003d48c0 reads mUnknown60 directly.
    friend class MetaGameWorld;

public:
    /**
     * Construct the renderer and start the front-end load.
     *
     * Records itself in the singleton pointer at `0x006c3598`, which the destructor clears, fills
     * the two debug-overlay switches from Script::QueryConfigString()'s integer sibling under codes
     * 0x397 and 0x3a2, sets the device clear colour to opaque black, and enqueues the three
     * container loads the boot phase of the poll routine waits on.
     *
     * @ghidraAddress 0x00369fb0
     */
    MetRenderer();

    /**
     * Release the screen stack, the two input helpers, and the fade.
     *
     * Slot 1 of all three tables. ~RendererBase and ~MsgSource are inlined into the body rather
     * than called, and the tagged release at the end passes `MsgSink` rather than the name of this
     * class, because MsgSink declares the allocation pair and this class inherits it.
     *
     * @ghidraAddress 0x0036a460
     */
    virtual ~MetRenderer();

    /**
     * Dispatch one front-end message.
     *
     * Slot 3 of the RendererBase table, where both RendererBase and MsgSink store the
     * `__pure_virtual` stub. Compares the message identity against seven registered identities and
     * forwards anything else to the active panel.
     *
     * A RawControllerMsg is decoded into a MetScreenCommand and delivered to the active panel. A
     * MetStartNetLaunchMsg and a LobbyConnectionLostMsg are forwarded to the active panel's
     * MsgSink::Handle(). A GameConnectionLostMsg is discarded. An IsRecordingMsg stores its payload
     * in mUnknown78.
     *
     * @param pMsg The message to dispatch.
     * @ghidraAddress 0x0036c5d8
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Start the front end running.
     *
     * RendererBase slot 4, whose verb the base does not supply. Clears the auto-repeat state, sets
     * mUnknowna8, rewinds the animation frame to 1.0f, and records the current time as the frame
     * base in mUnknown70.
     *
     * @ghidraAddress 0x0036a900
     */
    virtual void OnUnknownSlot4();

    /**
     * Stop the front end running.
     *
     * RendererBase slot 5, whose verb the base does not supply. Clears mUnknowna8, empties the
     * screen stack one element at a time, releases both scene views, clears the active panel, and
     * starts the front-end music when the object at `0x00699d70` reports state 2. The destructor
     * calls the routine directly.
     *
     * @ghidraAddress 0x0036b0c0
     */
    virtual void OnUnknownSlot5();

    /**
     * Advance the front end by one frame, including the boot and disc-problem phases.
     *
     * RendererBase slot 7, where the base stores the `__pure_virtual` stub. Four phases, each
     * guarded by its own flag. The boot phase waits for the three container loads, runs
     * OnUnknownSlot4() through the table, and makes `MetMemDetectStartup` the active panel. The
     * disc-problem phase starts the front-end music and promotes the pending panel. The third
     * phase shows or hides `met_disc_prob.view`. The frame phase advances the animation frame and
     * runs MetScreen::UpdateFrame() on every screen on the stack.
     *
     * @ghidraAddress 0x0036b190
     */
    virtual void OnUnknownSlot7();

    /**
     * Draw the front-end scene, every screen, and the debug overlays.
     *
     * RendererBase slot 8, where the base stores the `__pure_virtual` stub. Draws nothing at all
     * while mUnknowna8 is clear.
     *
     * @ghidraAddress 0x00371670
     */
    virtual void OnUnknownSlot8();

    /**
     * Advance every screen's animation frame.
     *
     * RendererBase slot 9, whose verb the base does not supply. The routine differs from
     * OnUnknownSlot7()'s frame phase in exactly one respect: it runs
     * MetScreen::UpdateAnimationFrame() where the poll routine runs MetScreen::UpdateFrame().
     *
     * @ghidraAddress 0x0036b740
     */
    virtual void OnUnknownSlot9();

    /**
     * Draw the front-end scene and the debug overlays, and no screen.
     *
     * RendererBase slot 10, whose verb the base does not supply. The routine is OnUnknownSlot8()
     * without the pre-pass and without the walk of the screen stack.
     *
     * @ghidraAddress 0x00371570
     */
    virtual void OnUnknownSlot10();

    /**
     * Promote the pending panel and start it entering.
     *
     * FadeUser slot 2. Does nothing while mUnknownc8 is set. The same five-step promotion appears
     * twice more inside OnUnknownSlot7().
     *
     * @ghidraAddress 0x003715e0
     */
    virtual void OnFadeOutDone();

    /**
     * Do nothing when a fade in finishes. FadeUser slot 3.
     *
     * The body is empty. It is a genuine override rather than an inherited empty body, because a
     * class cannot be concrete while a slot points at that stub, so this empty body is what makes
     * the renderer instantiable.
     *
     * @ghidraAddress 0x003715d8
     */
    virtual void OnFadeInDone();

    /**
     * Record one screen as the active panel.
     *
     * Stores pScreen in mUnknown7c and then, when the auto-repeat table is present, clears it.
     * The title is inferred from the field MetScreen slot 6 pairs the call with.
     *
     * @param pScreen The screen to record.
     * @ghidraAddress 0x003714c8
     */
    void SetActivePanel(MetScreen *pScreen);

    /**
     * Attach the three animatable, drawable, and transformable subobjects of one view to the
     * screen scene at mUnknowna0.
     *
     * Each of the three is appended only when the scene does not already store it, which the three
     * membership tests at `0x00370ab8`, `0x00370b08`, and `0x00370b58` decide. A null view is
     * passed through to all three as null rather than rejected. The title is inferred.
     *
     * @param pView The view to attach.
     * @ghidraAddress 0x003717b0
     */
    void AddScreenView(Rnd::View *pView);

    /**
     * Attach one view to the background scene at mUnknowna4.
     *
     * The body is AddScreenView() against the other scene, instruction for instruction, including
     * the three membership tests and the null pass-through. The title is inferred.
     *
     * @param pView The view to attach.
     * @ghidraAddress 0x003718b8
     */
    void AddBackgroundView(Rnd::View *pView);

    /**
     * Detach one view from the screen scene at mUnknowna0.
     *
     * The counterpart of AddScreenView(). The transformable, drawable, and animatable subobjects
     * are removed in that order, and a null view is passed through to all three as null. The title
     * is inferred.
     *
     * @param pView The view to detach.
     * @ghidraAddress 0x00371858
     */
    void RemoveScreenView(Rnd::View *pView);

    /**
     * Append one screen to the screen stack.
     *
     * A screen already on the stack is not appended a second time. The title is inferred from the
     * vector the body appends to.
     *
     * @param pScreen The screen to append.
     * @ghidraAddress 0x003719e0
     */
    void AddScreen(MetScreen *pScreen);

    /**
     * Erase one screen from the screen stack and detach its view from the scene.
     *
     * A screen absent from the stack does nothing. The title is inferred.
     *
     * @param pScreen The screen to erase.
     * @ghidraAddress 0x00371a78
     */
    void RemoveScreen(MetScreen *pScreen);

    /**
     * Make one screen the active panel and start it entering.
     *
     * The same promotion OnFadeOutDone() performs on the pending panel. The image records no
     * caller. The title is inferred.
     *
     * @param pScreen The screen to promote.
     * @ghidraAddress 0x003714f8
     */
    void ActivatePanel(MetScreen *pScreen);

    /**
     * Move a screen view to the front of the screen scene's draw order.
     *
     * A view the scene does not draw yet is attached through AddScreenView() instead. The image
     * records no caller. The title is inferred.
     *
     * @param pView The view to raise.
     * @ghidraAddress 0x00371730
     */
    void MoveScreenViewToFront(Rnd::View *pView);

    /**
     * Send a MetUnlockStagesMsg to the active panel, when there is one.
     *
     * The `ActivateAllAccessMode` script command is the caller. The title is inferred.
     *
     * @ghidraAddress 0x0036b8c8
     */
    void UnlockAllStages();

    /**
     * Report whether the active panel is a MetLogoScreen.
     *
     * The three cheat script commands test it. The title is inferred.
     *
     * @return Non-zero when the active panel is a MetLogoScreen.
     * @ghidraAddress 0x00371b58
     */
    int IsLogoScreenActive();

    /**
     * Hand a message to the active panel, when there is one.
     *
     * The image records no caller. The title is inferred.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00371cb0
     */
    void ForwardToPanel(Message *pMsg);

    /**
     * Hand a message to the active panel without testing for one.
     *
     * The image records no caller. The title is inferred.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00371cf0
     */
    void ForwardToPanelUnchecked(Message *pMsg);

    /**
     * Report how far the arena container load has advanced.
     *
     * MetSonyScreen slot 26 is the caller. The title is inferred.
     *
     * @param pfProgress Receives the load's progress, between 0 and 1.
     * @return Non-zero once the load is complete.
     * @ghidraAddress 0x003713f0
     */
    static int PollArenaLoader(float *pfProgress);

    /**
     * Report how far the three common container loads have advanced together.
     *
     * The image records no caller. The title is inferred.
     *
     * @param pfProgress Receives the mean of the three loads' progress.
     * @return Non-zero once all three are complete.
     * @ghidraAddress 0x00371338
     */
    static int PollCommonLoaders(float *pfProgress);

    /**
     * Enqueue each common container load that is still pending.
     *
     * The image records no caller. The title is inferred.
     *
     * @ghidraAddress 0x00371270
     */
    static void EnqueueCommonLoaders();

    /**
     * Unload the three common containers.
     *
     * The image records no caller. The title is inferred.
     *
     * @ghidraAddress 0x003712f8
     */
    static void UnloadCommonLoaders();

    /**
     * Unload the arena container.
     *
     * The image records no caller. The title is inferred.
     *
     * @ghidraAddress 0x00371490
     */
    static void UnloadArenaLoader();

    /**
     * Current animation frame position of the front end.
     *
     * MetScreen reads this field and passes it straight to its own enter and exit animation
     * virtuals at vtable slots 31 and 34, both of which take a float, and every screen that starts
     * a prompt or a title passes it on. The units are animation frames rather than seconds, which
     * mUnknown64 fixes.
     *
     * +0x68
     */
    float mUnknown68;

    /**
     * Flag that MetScreen sets when it activates a named sub-screen and clears when it activates
     * none.
     *
     * Written directly by MetScreen vtable slot 6 at `0x0038b828`, which stores 0 for the empty
     * name and 1 once the named screen reports that it has finished loading. No accessor for the
     * field appears in the image, so it is public. The constructor starts it at 1.
     *
     * +0x80
     */
    int mUnknown80;

private:
    // 0x0036a680
    // Resolves the three scene views and the fade, and is reached only from
    // OnUnknownSlot7()'s boot phase. The title is inferred from the three fields it writes.
    void ResolveSceneViews();

    // 0x0036a9e0
    // Resolves `Metagame_arena.view` and attaches it to the background scene. A
    // non-zero argument skips the resolve and attaches nothing, because the view pointer then
    // stays null. The title is inferred from the literal.
    void ResolveArenaView(int nSkipResolve);

    // 0x003719a0
    // Releases the animatable, drawable, and transformable lists of the screen scene
    // at mUnknowna0. OnUnknownSlot5() is its one caller. The title is inferred.
    void ClearScreenScene();

    // 0x00371960
    // Releases the same three lists of the background scene at mUnknowna4.
    // OnUnknownSlot5() is its one caller. The title is inferred.
    void ClearBackgroundScene();

    // 0x0036b938
    // Handles a MetStartPauseMsg. The body is not written.
    void OnStartPause(Message *pMsg);

    // 0x0036bcb8
    // Handles a MetFreqEndedMsg. The body is not written.
    void OnFreqEnded(Message *pMsg);

    // 0x0036aae0
    // Chooses the end-of-game screen from the game parameters, the game mode, and the solo
    // result. OnFreqEnded() is the caller. The title is inferred.
    MetScreen *SelectEndScreen();

    // 0x00371ba8
    // Translates a raw controller reading into a command, arms its auto-repeat, and delivers it
    // to the active panel. HandleMessage() expands it inline.
    inline void OnRawController(RawControllerMsg *pMsg);

    // 0x00369b08
    // Creates the metagame, fonts, and shared-texture loaders. The constructor is the caller.
    static void CreateCommonLoaders();

    // 0x00369e50
    // Creates the arena loader, builds the main-menu screens, and starts the arena load.
    static void CreateArenaLoader();

    // 0x003712d8
    static void StartArenaLoad();

    // 0x00371438. Enqueues the arena loader while it is pending.
    static void EnqueueArenaLoader();

    // 0x006c3598. The constructor records the renderer here and the destructor clears it.
    static MetRenderer *sInstance;
    // 0x006c3600
    static RndAsyncLoader *sMetagameLoader;
    // 0x006c3608
    static RndAsyncLoader *sFontsLoader;
    // 0x006c360c
    static RndAsyncLoader *sSharedTexLoader;
    // 0x006c3610
    static RndAsyncLoader *sArenaLoader;

    // +0x60. Zeroed by the constructor. MetaGameWorld::OnUnknownQuery003d48c0() reads it.
    int mUnknown60;
    // Rate the animation frame advances at, in frames per second. The constructor sets 500.0f, and
    // both frame routines compute `mUnknown68 += mUnknown64 * elapsedMilliseconds / 1000.0f`.
    float mUnknown64; // +0x64
    // +0x6c. The constructor does not write it and no reader is identified.
    int mUnknown6c;
    // Time the previous frame ran at, in nanoseconds since the watchdog's base. Both frame
    // routines difference it against the current time and then overwrite it.
    long long mUnknown70; // +0x70
    // Payload of the last IsRecordingMsg. HandleMessage() is the one writer.
    int mUnknown78; // +0x78
    // The screen that receives decoded commands. SetActivePanel() is the named writer, and the two
    // promotion sequences write it directly.
    MetScreen *mUnknown7c; // +0x7c
    // Every screen the front end is showing, in the order it was pushed. AddScreen() appends and
    // RemoveScreen() erases, and both set mUnknown98 afterwards.
    std::vector<MetScreen *> mUnknown84; // +0x84
    // Translates a controller reading into a command. Allocated by the constructor as twelve bytes
    // and released by the destructor. The class name is inferred, for the reason its own header
    // records.
    MetCommandMap *mUnknown90; // +0x90
    // One auto-repeat record per controller. Allocated by the constructor as twelve bytes and
    // released by the destructor. The class name is inferred on the same basis.
    MetCommandRepeater *mUnknown94; // +0x94
    // Set by AddScreen() and RemoveScreen() once either has changed mUnknown84. Both frame
    // routines abandon their walk of the stack when they observe it, because the change
    // invalidated the iterator they were holding.
    int mUnknown98; // +0x98
    // `met top view`, the front-end shell. Both draw routines draw it and both frame routines set
    // its frame.
    Rnd::View *mUnknown9c; // +0x9c
    // `metscreens.view`, the scene AddScreenView() attaches a screen's view to.
    Rnd::View *mUnknowna0; // +0xa0
    // `meta bg view`, the scene AddBackgroundView() attaches to.
    Rnd::View *mUnknowna4; // +0xa4
    // Set while the front end is running. Every draw and frame routine returns at once when it is
    // clear.
    int mUnknowna8; // +0xa8
    // Draw the subsystem timing graph. Filled from configuration code 0x397.
    int mUnknownac; // +0xac
    // Draw the render-statistics overlay. Filled from configuration code 0x3a2.
    int mUnknownb0; // +0xb0
    // +0xb4. The constructor sets 1 and no reader is identified.
    int mUnknownb4;
    // Set while the three boot container loads are outstanding. The poll routine clears it once
    // all three report complete.
    int mUnknownb8; // +0xb8
    // +0xbc. Zeroed by the constructor and read nowhere that has been identified.
    int mUnknownbc;
    // +0xc0. A std::list, which in this template library is one pointer to a self-linked dummy
    // node. The constructor creates that node inline under the allocation tag `stl_list` for a
    // four-byte element, and the destructor clears the list through 0x00272ee8 and returns the
    // node. Nothing that has been read appends to it or walks it, so the element type is not
    // recovered and the member is recorded as the four bytes it occupies.
    unsigned char mUnknownc0[4];
    // The screen the next fade promotes to the active panel.
    MetScreen *mUnknownc4; // +0xc4
    // Set while the disc-problem phase of the poll routine has work outstanding.
    int mUnknownc8; // +0xc8
    // The fade, allocated by ResolveSceneViews() as forty-four bytes. The destructor releases it
    // with the scalar free rather than through a destructor, which is what a class with no virtual
    // and no member needing teardown compiles to.
    MetFade *mUnknowncc; // +0xcc
    // Set while a fade is running. Both the poll routine and the fade-finished override return
    // early on it rather than promoting a panel.
    int mUnknownd0; // +0xd0
    // Highest pad index HandleMessage() accepts a RawControllerMsg from. The constructor sets 4,
    // and the test is `mUnknownd4 < padIndex`, so index 4 is accepted and index 5 is not.
    int mUnknownd4; // +0xd4
};
