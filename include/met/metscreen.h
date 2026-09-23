#pragma once

#include <list>
#include <map>
#include <vector>

#include "app/msgsink.h"
#include "met/metrenderer.h"
#include "os/hxstr.h"
#include "rnd/asyncloader.h"
#include "rnd/drawable.h"
#include "rnd/object.h"
#include "rnd/view.h"

/**
 * One shared container load, interned under the container name.
 *
 * The record is 12 bytes and is not polymorphic, so it emits no RTTI and its title is inferred
 * from its one use rather than recovered. MetScreen::BeginContainerLoad() allocates one per
 * distinct container name and MetScreen::SetShowing() and MetScreen::PollContainerLoad() read it
 * back. Two screens loading the same container therefore share one RndAsyncLoader.
 *
 * The two integers are recorded as unrecovered. BeginContainerLoad() writes both and tests
 * mUnknown08, and nothing else in the image reads either.
 */
struct MetContainerLoad {
    RndAsyncLoader *mLoader; /*!< The request the container loads through. +0x00 */
    int mUnknown04;          /*!< +0x04 */
    int mUnknown08;          /*!< +0x08 */
};

/**
 * One navigation command delivered to a screen.
 *
 * The record is 12 bytes and is not polymorphic, so it emits no RTTI and its title is inferred
 * from its use rather than recovered. MetRenderer::HandleMessage builds one on its own stack from
 * an input message through the translator at `0x002e3738` and passes it to
 * MetScreen::DeliverCommand(). The three fields come straight from that translator, which copies
 * the pad index and the button identifier out of the input message and maps the button to a
 * command code.
 */
struct MetScreenCommand {
    int mCommand;  /*!< What the user requested, from MetScreenCommandCode. +0x00 */
    int mPadIndex; /*!< Which controller produced the command. +0x04 */
    int mButton;   /*!< Raw button identifier the translator mapped. +0x08 */
};

/**
 * What a navigation command requests.
 *
 * The six values are the ones MetScreen::DeliverCommand() routes to a sound, through a six-entry
 * jump table at `0x0080b460` indexed by the command code less one. Left and right are fixed by
 * the sound literals that branch plays, `SND_MET_CYCLE_L` and `SND_MET_CYCLE_R`. Previous and next
 * both play `SND_MET_HIGH`, and MetMainScreen separates them at `0x002c6820` by routing one to
 * MetButtonList vtable slot 2 and the other to slot 3, the two navigation virtuals that walk the
 * button ring in opposite directions. Which of the two moves which way is not recovered.
 *
 * The translator at `0x002e3738` produces further codes from a 103-entry table, among them 7 and
 * 15. A code above six is discarded by DeliverCommand() without a sound and passed to slot 19
 * unchanged, and its meaning is recovered nowhere in the image.
 */
enum MetScreenCommandCode {
    kMetScreenCommandNone = 0, /*!< The renderer discards the command instead of delivering it. */
    kMetScreenCommandPrevious = 1, /*!< One step along the button ring. */
    kMetScreenCommandNext = 2,     /*!< One step the other way along the button ring. */
    kMetScreenCommandLeft = 3,     /*!< Cycle the selected value to the left. */
    kMetScreenCommandRight = 4,    /*!< Cycle the selected value to the right. */
    kMetScreenCommandSelect = 5,   /*!< Act on the selection. */
    kMetScreenCommandBack = 6      /*!< Depart the screen. */
};

/**
 * Base of every front-end screen.
 *
 * `9MetScreen` in the RTTI descriptor at `0x008eed98`, with MsgSink as its one public non-virtual
 * base at offset 0. The object is 0x8c bytes, which MetMemDetectScreen, MetSaveRemix,
 * MetSoloWinScreen, and seven further classes fix by placing their second base at `+140`. The
 * vtable is at `0x0080b6a0`. A walk of the table reads 39 entries, slots 0 to 38, followed by an
 * all-zero terminating entry at `0x0080b838` and then the next class's table at `0x0080b840`. The
 * terminator is not a slot.
 *
 * Thirty-two classes derive directly from the class, and five intermediate classes sit between it
 * and their own children, MetGizmoPanel, MetJukeboxBaseScreen, MetMultiTipsBaseScreen,
 * MetPauseBaseScreen, and MetScreenMultiSoundBank.
 *
 * A screen manages a Rnd::View loaded from a `.rnd` container. The constructor records the
 * container name and, when both name arguments are non-empty, starts the load with a direct call to
 * slot 18's body rather than a dispatch through the table. That is what a virtual call from a
 * constructor compiles to, and the destructor invokes slot 13 the same way.
 *
 * Slot 14 polls the RndAsyncLoader until the load completes and then runs slot 38, which resolves
 * the view by appending `.view` to the container name and resolves the two animation views by
 * formatting `%s_EE.anim` and `%s_BF.anim` from the screen name. A screen with no view trips the
 * diagnostic `the screen %s doesn't have a valid view!`.
 *
 * Every float this class passes or receives for an animation is a frame position rather than a time
 * in seconds. MetRenderer advances the field at its own `+0x68` by a rate at `+0x64` that defaults
 * to 500, and hands the result to Rnd::Animatable::SetFrame(). That is what makes the arithmetic
 * here coherent: mUnknown08 and mUnknown0c store frame positions, mUnknown04 stores the enter
 * animation's end frame from Rnd::View::EndFrame(), and UpdateEnterAnimation() subtracts one from
 * the sum of the other two. An earlier reading described the argument as a time in seconds, which
 * would have mixed units across that expression.
 *
 * The 39 entries follow in table order. Slot 0 is the compiler-generated GetTypeInfo at
 * `0x0038fd78` and is not source. An entry marked empty is a two-instruction `jr ra` stub. Each
 * such stub sits at its own address and several derived tables address the same stub, so the stub
 * is an out-of-line definition rather than an inline body, and this class owes one definition for
 * each.
 *
 *  - 1 `0x0038a848` the destructor.
 *  - 2 `0x00105158` MsgSink::Handle(), inherited unchanged.
 *  - 3 `0x003907a8` MsgSink::HandleMessage(), overridden empty.
 *  - 4 `0x00390200` PushNamedScreen().
 *  - 5 `0x003900a8` EnterAndShow().
 *  - 6 `0x0038b828` ActivateNamedPanel().
 *  - 7 `0x0038fdf8` OnUnknownSlot7(), empty.
 *  - 8 `0x003902d0` ExitScreenByName().
 *  - 9 `0x00390100` BeginExit().
 *  - 10 `0x00390130` OnUnknownSlot10(), empty and unrecovered.
 *  - 11 `0x00390138` OnKeyboardDismissed(), empty.
 *  - 12 `0x0038fe00` OnDrawPass(), empty.
 *  - 13 `0x003900a0` OnDestroying(), empty.
 *  - 14 `0x0038b338` PollContainerLoad().
 *  - 15 `0x0038fe20` OnMsgScreenDismissed(), empty.
 *  - 16 `0x0038fe28` OnMsgScreenShown(), empty.
 *  - 17 `0x0038b490` SetShowing().
 *  - 18 `0x0038aa00` BeginContainerLoad().
 *  - 19 `0x0038fe30` HandleCommand(), empty.
 *  - 20 `0x00390140` PlaySlideSound().
 *  - 21 `0x00390160` PlayLeaveSound().
 *  - 22 `0x003901c0` PlayHighSound().
 *  - 23 `0x00390180` PlayCycleLeftSound().
 *  - 24 `0x003901a0` PlayCycleRightSound().
 *  - 25 `0x003901e0` PlayErrorSound().
 *  - 26 `0x0038fe38` OnUnknownSlot26(), empty.
 *  - 27 `0x0038fe40` UpdateIdleAnimation(), empty.
 *  - 28 `0x00390498` StartRepeatingSound().
 *  - 29 `0x003904e0` UpdateRepeatingSound().
 *  - 30 `0x0038fe48` OnUnknownSlot30(), empty.
 *  - 31 `0x003905c0` StartEnterAnimation().
 *  - 32 `0x003905f0` UpdateEnterAnimation().
 *  - 33 `0x0038fe50` OnUnknownSlot33(), empty.
 *  - 34 `0x003906a0` StartExitAnimation().
 *  - 35 `0x003906b0` UpdateExitAnimation().
 *  - 36 `0x0038fe58` OnUnknownSlot36(), empty.
 *  - 37 `0x00390788` Draw().
 *  - 38 `0x0038b1b0` ResolveContainerViews().
 *
 * A walk of all 78 tables in the image that share at least twelve entries with this one settles
 * which of the empty slots a derived class fills. Slot 19 is filled 53 times, slot 15 23 times,
 * slot 27 6 times, slot 16 twice, and slot 11 once. Slots 10, 12, and 13 are filled by no derived
 * table at all.
 *
 * Five of the six sound slots take one integer argument. MetKeyboardScreen::StartRepeatingSound()
 * at `0x0028c5e0` loads its own selector into `a1` immediately before each of its five virtual
 * calls, to slots 20, 22, 23, 24, and 25, so the argument is set at a call site rather than
 * inherited from a register. The same class overrides four of the five and each override reads `a1`
 * and compares it against the selector it recorded.
 *
 * The selector is the controller index. DeliverCommand() loads it from `+0x04` of the command
 * record, MetRenderer::HandleMessage rejects a command whose `+0x04` exceeds its own bound at
 * `+0xd4` before delivering it, and the translator at `0x002e3738` copies that word out of the
 * input message unchanged. A screen that records -1 plays its sound for every controller.
 *
 * PlayLeaveSound() is declared here with no argument and the evidence now contradicts that.
 * DeliverCommand() loads `a1` from the same `+0x04` for slot 21 at `0x0038b7e8` exactly as it does
 * for the other five, and the MetFreqMakerInventoryScreen override at `0x00272bb8` that an earlier
 * reading treated as an argument-free forward compiles identically either way, because the value
 * already sits in `a1`. The declaration is unchanged here because nine derived headers in other
 * files declare the override without the argument, and correcting one file alone would produce a
 * base that its subclasses no longer override.
 *
 * Four data members are protected and the rest are private. MetRemixLoadScreen and
 * MetRemixDelScreen both clear mUnknown60 in their constructors, and MetSaveRemixScreen clears
 * mUnknown5c in its own. Four derived constructors append to mUnknown38, and
 * MetJukeboxBaseScreen::SetShowing() reads mUnknown48. Those four are written or read by derived
 * code and the others are not. Nothing outside the class and its children reads any member.
 */
class MetScreen : public MsgSink {
public:
    /**
     * Construct a screen and start its container load.
     *
     * The load starts only when HxStr::DiffersFromLiteral reports both directory and file different
     * from the empty literal. BeginContainerLoad() is invoked with a direct call rather than
     * through vtable slot 18, which is what a virtual call from a constructor compiles to. An
     * earlier reading recorded a dispatched call and a registration that happens last, and both are
     * wrong. MsgSource::AddSink() runs before the load starts, not after it.
     *
     * mUnknown04, mUnknown70, and mUnknown74 are not written, so a screen starts with three
     * indeterminate fields. Slot 38 writes mUnknown04 and slot 28 writes the other two.
     *
     * The body is not written. Every part is recovered: mUnknown10 takes pRenderer, mUnknown18
     * takes 2, mUnknown58 takes 1.0f, mUnknown48, mUnknown5c, and mUnknown60 each take 1,
     * mUnknown20 is copy-constructed from name, mUnknown80 from file, mUnknown88 from nPriority,
     * every remaining integer, float, and pointer member is zeroed, mUnknown28 is assigned `file`
     * with `.rnd` appended, `mUnknown10->AddSink(this)` registers the screen, and the load then
     * starts. What blocks the body is MetRenderer's own header, which declares the class with no
     * base and a reserved byte array over `+0x00` through `+0x67`, so AddSink() is unreachable
     * through the member. The RTTI descriptor lists MsgSource at offset 0, RendererBase at 20, and
     * FadeUser at 92, and the body follows once MetRenderer declares them.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority, passed to RndAsyncLoader unchanged.
     * @param name The screen name, from which the animation view names are formatted.
     * @param directory The directory the container loads from.
     * @param file The container name, without the `.rnd` suffix.
     * @ghidraAddress 0x0038a450
     */
    MetScreen(MetRenderer *pRenderer,
              int nPriority,
              const HxStr &name,
              const HxStr &directory,
              const HxStr &file);

    /**
     * Unregister from the renderer and release the container.
     *
     * Slot 1. The hand-written part is three calls and nothing else. Everything the disassembly
     * shows besides them is compiler-generated: the two table-pointer stores, the inlined `HxStr`
     * destructors for mUnknown20, mUnknown28, and mUnknown80, the `std::vector` and `std::list`
     * teardown, and the conditional release under the `__in_chrg` flag.
     *
     * The body is not written, and the three calls are
     * `mUnknown10->RemoveScreen(this)`, `OnDestroying()`, and
     * `mUnknown10->RemoveSink(this)`, in that order. RemoveSink() blocks it for the reason recorded
     * on the constructor.
     *
     * @ghidraAddress 0x0038a848
     */
    virtual ~MetScreen();

    /**
     * Shared table of container loads, interned under the container name.
     *
     * The table is a function-local static, so the accessor is the only route to it and the
     * compiler wrapped it in the guard flag at `0x006c64d8`. The tree it builds is tagged
     * `stl_maptree` with a 12-byte value, and its header node is 0x20 bytes, which is 16 bytes of
     * tree header plus the 8-byte key and the 4-byte pointer.
     *
     * @return The table.
     * @ghidraAddress 0x00381e10
     */
    static std::map<HxStr, MetContainerLoad *> &ContainerLoaderMap();

    /**
     * Shared table of live screens, interned under the screen registry key.
     *
     * The key is the screen class name as a literal, and the literal is authoritative rather than
     * derivable from the class, because at least one screen registers under a spelling that
     * differs from its class name. The table is a function-local static behind the guard flag at
     * `0x006c64dc`, with a 16-byte value and a 0x20-byte header node.
     *
     * @return The table.
     * @ghidraAddress 0x003821e0
     */
    static std::map<HxStr, MetScreen *> &ScreenRegistry();

    /**
     * Resolve one screen by its registry key.
     *
     * @param name The registry key.
     * @return The screen, or null when no screen has registered under the key.
     * @ghidraAddress 0x0038ff90
     */
    static MetScreen *FindScreenByName(const HxStr &name);

    /**
     * Play the sound one navigation command calls for and then act on the command.
     *
     * Not a vtable slot. MetRenderer::HandleMessage at `0x0036c6e8` and three further routines
     * arrive at it with a direct call. A screen whose mUnknown1c is clear ignores the command
     * completely, and a screen whose mUnknown5c is clear skips the sound and acts on the command
     * regardless. The sound is chosen through a six-entry jump table at `0x0080b460` indexed by the
     * command code less one, and every branch of that table passes MetScreenCommand::mPadIndex to
     * the sound slot it runs. Slot 19 then receives the whole record.
     *
     * Declared public because MetRenderer calls it from outside the hierarchy and the image has
     * no other route to it. A friend declaration fits the image equally well.
     *
     * @param pCommand The command the renderer translated from an input message.
     * @ghidraAddress 0x0038b730
     */
    void DeliverCommand(const MetScreenCommand *pCommand);

    /**
     * Drive the frame of whichever animation is running and run the idle hook when none is.
     *
     * Not a vtable slot. MetRenderer::Slot9 at `0x0036b858` is its one caller. A screen still
     * waiting for its container load, which is what a set mUnknown4c records, does nothing here.
     * Both branches drive mUnknown30 rather than mUnknown34, matching UpdateExitAnimation().
     *
     * Declared public for the same reason as DeliverCommand().
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress 0x00390380
     */
    void UpdateAnimationFrame(float flTime);

    /**
     * Advance one screen by one frame.
     *
     * Not a vtable slot. MetRenderer::Slot7 at `0x0036b650` is its one caller. The routine has two
     * disjoint halves and mUnknown4c selects between them. A screen still waiting for its container
     * load, which is what a set mUnknown4c records, takes the deferred-entry half and no animation
     * runs. Every other screen takes the animation half.
     *
     * The deferred half consults the container load interned under mUnknown28. Once that load
     * reports finished, slot 38 resolves the views while mUnknown48 is set, the view is handed to
     * the renderer, slot 5 enters the screen, and a screen that also has mUnknown50 set becomes the
     * active panel and runs slot 7. While the load is unfinished the half polls the RndAsyncLoader
     * instead and records the completion in the shared load record, which is the one place
     * MetContainerLoad::mUnknown04 is written after BeginContainerLoad().
     *
     * The animation half runs slot 32, then slot 26 either when mUnknown54 is set or when neither
     * animation is running, then slot 29 and slot 35. Slot 26 therefore fires on the same
     * both-times-zero condition that UpdateAnimationFrame() uses for slot 27, which is what pairs
     * the two idle hooks across the renderer's two passes.
     *
     * An earlier reading titled this routine for the deferred half alone and recorded the
     * mUnknown4c test inverted. The animation half is the common path, not the exceptional one.
     *
     * Declared public for the same reason as DeliverCommand().
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress 0x0038b918
     */
    void UpdateFrame(float flTime);

    /**
     * Bring one named screen onto the renderer's screen stack.
     *
     * Slot 4. The screen is appended to the stack first and only enters once its own slot 14
     * reports the container load finished. A screen whose load has not finished instead records 1
     * in mUnknown4c and is entered by a later call.
     *
     * @param name The registry key of the screen to push.
     * @ghidraAddress 0x00390200
     */
    virtual void PushNamedScreen(const HxStr &name);

    /**
     * Show this screen and start its enter animation.
     *
     * Slot 5.
     *
     * @ghidraAddress 0x003900a8
     */
    virtual void EnterAndShow();

    /**
     * Make one named screen the renderer's active panel.
     *
     * Slot 6. An empty name clears MetRenderer::mUnknown80 and activates nothing. A screen whose
     * slot 14 reports the load unfinished instead records 1 in its own mUnknown50.
     *
     * @param name The registry key of the panel to activate, or an empty string for none.
     * @ghidraAddress 0x0038b828
     */
    virtual void ActivateNamedPanel(const HxStr &name);

    /**
     * Unrecovered. Slot 7.
     *
     * The body is empty and slot 6 is its one caller, which passes no argument. Neither its
     * purpose nor a wider argument list can be established.
     *
     * @ghidraAddress 0x0038fdf8
     */
    virtual void OnUnknownSlot7();

    /**
     * Start one named screen's exit animation.
     *
     * Slot 8. Writes `Exiting screen: %s` to the memory log first. The resolved screen is used
     * without a null check, so a key that no screen registered under faults.
     *
     * @param name The registry key of the screen to exit.
     * @ghidraAddress 0x003902d0
     */
    virtual void ExitScreenByName(const HxStr &name);

    /**
     * Start this screen's exit animation at the renderer's current time.
     *
     * Slot 9.
     *
     * @ghidraAddress 0x00390100
     */
    virtual void BeginExit();

    /**
     * Unrecovered. Slot 10.
     *
     * The body is empty. No derived table among the 78 in the image fills the slot, and no routine
     * in the image dispatches it on a screen. Every one of the 42 sites that loads a table entry at
     * this index dispatches it on the object Globals::GetGameManager() vends or on an unrelated
     * class, and the destructor's direct call to slot 13 has no counterpart here. Neither the
     * purpose nor the argument list is recovered.
     *
     * @ghidraAddress 0x00390130
     */
    virtual void OnUnknownSlot10();

    /**
     * Take control back after the keyboard screen has finished.
     *
     * Slot 11. The body is empty. MetKeyboardScreen::OnUnknownSlot36() at `0x00283b74` is the one
     * caller in the image: once the entered text is committed it resolves the screen whose registry
     * key it recorded, runs this slot on that screen with no argument, and then makes the same key
     * its active panel. MetLoadNewFreqScreen fills the slot at `0x002a3978` and re-pushes two named
     * screens and re-activates its panel, which is what fixes the hook as a resumption rather than
     * a report of the entered text.
     *
     * @ghidraAddress 0x00390138
     */
    virtual void OnKeyboardDismissed();

    /**
     * Contribute to the renderer's draw pass.
     *
     * Slot 12. The body is empty and no derived table among the 78 fills it, so the hook exists and
     * the shipped game implements it nowhere. MetRenderer::Slot8 at `0x003716c8` is the one caller
     * on a screen: it draws its own root drawable, walks the screen vector at `+0x84` running this
     * slot on each entry with no argument, and then draws the timing graph and the statistics
     * overlay.
     *
     * @ghidraAddress 0x0038fe00
     */
    virtual void OnDrawPass();

    /**
     * Release what the screen acquired, ahead of the rest of destruction.
     *
     * Slot 13. The body is empty and no derived table among the 78 fills it. The destructor at
     * `0x0038a890` is its one caller anywhere in the image, with a direct call and no argument,
     * placed after the screen unregisters from the renderer and before any member is released. A
     * direct rather than dispatched call is what a virtual invoked from a destructor compiles to,
     * because the dynamic type there is this class.
     *
     * @ghidraAddress 0x003900a0
     */
    virtual void OnDestroying();

    /**
     * Advance this screen's container load and resolve its views once the load finishes.
     *
     * Slot 14. The views are resolved only while mUnknown48 is set, which slot 38 clears, so the
     * resolution happens once. The report does not depend on mUnknown48.
     *
     * @return Non-zero once the container load has finished.
     * @ghidraAddress 0x0038b338
     */
    virtual int PollContainerLoad();

    /**
     * Act on the choice the user made in a message dialogue.
     *
     * Slot 15. The body is empty. MetMsgScreen::Slot36() at `0x002f05f4` is the one caller on a
     * screen: once its own exit animation has finished it clears its `+0xd8` flag and runs this
     * slot on the screen at its `+0xb8`, passing the `HxStr` at its `+0xb0` and the integer at its
     * `+0xd4`. Twenty-three derived tables fill the slot, and each body compares the first argument
     * against dialogue names through HxStr::MatchesLiteral and then branches on the second against
     * 0, 1, and 2. MetExpansionPakScreen at `0x002193e8` matches `expansion_prepare`,
     * `expansion_check`, `expansion_done`, `expansion_load`, and `expansion_retry`, and
     * MetMemDetectScreen at `0x002d9e40` matches `mem_check`, `mem_format_check`, `mem_error`, and
     * eight further keys. The second argument is which button the user chose, on the evidence that
     * those screens build their dialogues with the button sets `CONTINUE`/`CANCEL`/`RETRY` and
     * `YES`/`NO` and then branch on 0, 1, and 2. The MetSaveRemix override at `0x00375590` reads
     * both arguments before writing either, passing the first to HxStr::MatchesLiteral and
     * comparing the second against 1 at `0x003755b4`, which confirms the second argument
     * independently of the caller.
     *
     * @param name The dialogue the screen requested, which the message screen reports back.
     * @param nChoice Which of the dialogue's buttons the user chose, counted from zero.
     * @ghidraAddress 0x0038fe20
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Act on a message dialogue becoming visible.
     *
     * Slot 16. The body is empty. MetMsgScreen::Slot33() at `0x002f0524` is the one caller on a
     * screen: once its own enter animation has finished it sets its `+0xd8` flag and runs this slot
     * on the screen at its `+0xb8`, passing only the `HxStr` at its `+0xb0`. No second argument is
     * set. Two derived tables fill the slot. MetExpansionPakScreen at `0x0021d9e0` matches the name
     * against `expansion_prepare` and `expansion_load` and records which dialogue is up, and
     * MetRemixDelScreen at `0x003441a0` ignores the argument and activates a named panel.
     *
     * @param name The dialogue the screen requested, which the message screen reports back.
     * @ghidraAddress 0x0038fe28
     */
    virtual void OnMsgScreenShown(const HxStr &name);

    /**
     * Show or hide the view and every drawable the container loaded.
     *
     * Slot 17. Forwards to Rnd::Drawable::SetShowing() on mUnknown14 and then, when mUnknown60 is
     * set, on each entry of the drawable list that the loader recorded for mUnknown28. The name is
     * inferred from the Rnd::Drawable virtual it forwards to.
     *
     * The second half copies RndAsyncLoader::mDrawables out of the interned container load into a
     * temporary and walks it. The helper at `0x0014d560` that the copy expands to is
     * `std::list::insert` over a range and is library code, so the source wrote a copy construction
     * rather than a call. The view is dereferenced with no null check.
     *
     * @param nShowing Non-zero to draw the screen.
     * @ghidraAddress 0x0038b490
     */
    virtual void SetShowing(int nShowing);

    /**
     * Intern a container load under this screen's container name and enqueue it.
     *
     * Slot 18. A separator is appended to the directory before the request is built. The file
     * argument is declared and ignored, because the request is built from mUnknown28 instead.
     *
     * @param directory The directory the container loads from.
     * @param file Declared and ignored.
     * @ghidraAddress 0x0038aa00
     */
    virtual void BeginContainerLoad(const HxStr &directory, const HxStr &file);

    /**
     * Act on a navigation command.
     *
     * Slot 19. The body is empty. DeliverCommand() is the one caller, and it passes the whole
     * command record after playing the sound the command code calls for. Fifty-three derived tables
     * fill the slot, more than any other, and every body inspected reads MetScreenCommand::mCommand
     * and branches on it. MetMainScreen at `0x002c6820` routes command 1 to MetButtonList vtable
     * slot 2, command 2 to slot 3, command 5 to a named panel, and command 6 elsewhere.
     * MetConfigControllerScreen fills it at `0x00200ba8` and MetKeyboardScreen at `0x00283268`.
     *
     * @param pCommand The command the renderer translated from an input message.
     * @ghidraAddress 0x0038fe30
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the sound that accompanies sliding between screens.
     *
     * Slot 20. Passes the literal `SND_MET_SLIDE` to the named-sound player at `0x0012f470`. The
     * name is inferred from that literal. MetScreenMultiSoundBank, MetMultiTipsBaseScreen,
     * MetPauseBaseScreen, and MetJukeboxBaseScreen all override this slot, the last three with an
     * empty body, which is what proves the return type is void.
     *
     * The body ignores its argument. DeliverCommand() routes command 5 to this slot.
     *
     * @param nSelector The controller index the command came from, which an override compares
     *                  against its own recorded selector.
     * @ghidraAddress 0x00390140
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play the sound that accompanies departing a screen.
     *
     * Slot 21. Passes the literal `SND_MET_LEAVE` to the named-sound player at `0x0012f470`.
     * DeliverCommand() routes command 6 to this slot, loading the controller index into `a1`
     * first. The declaration retains no parameter for the reason recorded on the class.
     *
     * @ghidraAddress 0x00390160
     */
    virtual void PlayLeaveSound();

    /**
     * Play the sound that accompanies the emphasised selection.
     *
     * Slot 22. Passes the literal `SND_MET_HIGH` to the named-sound player at `0x0012f470`. The
     * body ignores its argument. DeliverCommand() routes commands 1 and 2 to this slot.
     *
     * @param nSelector The controller index the command came from, which an override compares
     *                  against its own recorded selector.
     * @ghidraAddress 0x003901c0
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * Play the sound that accompanies cycling a value to the left.
     *
     * Slot 23. Passes the literal `SND_MET_CYCLE_L` to the named-sound player at `0x0012f470`. The
     * body ignores its argument. DeliverCommand() routes command 3 to this slot, which is what
     * identifies command 3 as the leftward cycle.
     *
     * @param nSelector The controller index the command came from, which an override compares
     *                  against its own recorded selector.
     * @ghidraAddress 0x00390180
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play the sound that accompanies cycling a value to the right.
     *
     * Slot 24. Passes the literal `SND_MET_CYCLE_R` to the named-sound player at `0x0012f470`. The
     * body ignores its argument. DeliverCommand() routes command 4 to this slot, which is what
     * identifies command 4 as the rightward cycle.
     *
     * @param nSelector The controller index the command came from, which an override compares
     *                  against its own recorded selector.
     * @ghidraAddress 0x003901a0
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Play the sound that accompanies a rejected input.
     *
     * Slot 25. Passes the literal `SND_MET_ERROR` to the named-sound player at `0x0012f470`. This
     * is the one sound of the six that MetScreenMultiSoundBank does not override. The body ignores
     * its argument, and DeliverCommand() routes no command code to this slot.
     *
     * @param nSelector The controller index the command came from, which an override compares
     *                  against its own recorded selector.
     * @ghidraAddress 0x003901e0
     */
    virtual void PlayErrorSound(int nSelector);

    /**
     * Unrecovered. Slot 26.
     *
     * The body is empty here. MetKeyboardScreen fills it at `0x0028c708` with a body that reads its
     * argument out of `f12` and compares a float member against it, which is what fixes the single
     * parameter as a float. Nine further screens fill the slot too. The argument is the renderer
     * time, on the same evidence that fixes it for the two animation slots, because the
     * MetKeyboardScreen body adds a fixed 240 to a recorded value and tests the sum against it.
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress 0x0038fe38
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Advance whatever the screen animates while it is neither entering nor exiting.
     *
     * Slot 27. The body is empty. UpdateAnimationFrame() is the one caller on a screen, and it runs
     * the slot only while both mUnknown08 and mUnknown0c are zero, which is the interval after the
     * enter animation has finished and before the exit animation starts. Six derived tables fill
     * the slot. MetGizmoPanel at `0x0027b388` forwards to slot 26 unchanged, which is what pairs
     * the two hooks and confirms the float. MetLogoScreen at `0x002be5a8` toggles one object every
     * 120 units of renderer time and sets an animation view's frame to the time it received.
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress 0x0038fe40
     */
    virtual void UpdateIdleAnimation(float flTime);

    /**
     * Start alternating one object between two material states.
     *
     * Slot 28. A null object records nothing and starts nothing. The step count is doubled,
     * because one full cycle of the alternation is two steps.
     *
     * The body is not written. The alternation runs through the instance method at `0x00534a48`
     * on the recorded object, and the class that method belongs to cannot be titled. It reads a
     * current state at `+0x1c`, a `Rnd::Mesh` at `+0x20`, a drawable at `+0x24`, and two
     * state-indexed arrays at `+0x28` and `+0x34`, and neither RTTI, an embedded path, nor a
     * method name for it survives anywhere in the image.
     *
     * @param flStartTime The frame position the first step runs at.
     * @param flInterval The interval between steps.
     * @param pObject The object whose material state alternates.
     * @param nCycles The number of full cycles to run.
     * @ghidraAddress 0x00390498
     */
    virtual void
    StartRepeatingSound(float flStartTime, float flInterval, Rnd::Object *pObject, int nCycles);

    /**
     * Advance the alternation that StartRepeatingSound() started.
     *
     * Slot 29. One step runs per elapsed interval. The last step restores state 1, hands the
     * object to slot 30, and clears the three fields that drive the alternation.
     *
     * The body is not written, for the reason recorded on StartRepeatingSound().
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress 0x003904e0
     */
    virtual void UpdateRepeatingSound(float flTime);

    /**
     * Unrecovered. Slot 30.
     *
     * The body is empty. Slot 29 passes the object it finished alternating, and
     * MetConfigOptionsButtonsScreen overrides the slot at `0x00207fc0` with a body that copies the
     * `HxStr` at `+0x04` of the same argument. The argument is therefore one pointer, and its type
     * is recorded as the class that slot 29 passes.
     *
     * @param pObject The object slot 29 finished with.
     * @ghidraAddress 0x0038fe48
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Rewind the enter animation and record the time it starts at.
     *
     * Slot 31. The exit animation start time is cleared, so the two animations never run together.
     *
     * @param flTime The frame position the animation starts at.
     * @ghidraAddress 0x003905c0
     */
    virtual void StartEnterAnimation(float flTime);

    /**
     * Drive the enter animation and finish it once it passes its end.
     *
     * Slot 32. The end is detected on one call and acted on the next, which is what mUnknown7c
     * records between the two.
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress 0x003905f0
     */
    virtual void UpdateEnterAnimation(float flTime);

    /**
     * Unrecovered. Slot 33.
     *
     * The body is empty. Slot 32 runs it with no argument once the enter animation has finished,
     * and the MetConfigControllerScreen override at `0x002069d0` reads none either.
     *
     * @ghidraAddress 0x0038fe50
     */
    virtual void OnUnknownSlot33();

    /**
     * Record the time the exit animation starts at.
     *
     * Slot 34. Clears the enter animation start time and mUnknown1c. The title is inferred to
     * pair with UpdateExitAnimation().
     *
     * @param flTime The frame position the animation starts at.
     * @ghidraAddress 0x003906a0
     */
    virtual void StartExitAnimation(float flTime);

    /**
     * Drive the exit animation and hide the screen once it passes its end.
     *
     * Slot 35. The end is detected on one call and acted on the next, through mUnknown78. The
     * screen is hidden, erased from the renderer's stack, and then slot 36 runs.
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress 0x003906b0
     */
    virtual void UpdateExitAnimation(float flTime);

    /**
     * Unrecovered. Slot 36.
     *
     * Recorded on the same evidence as OnUnknownSlot33(), with the MetConfigControllerScreen
     * override at `0x00201790`.
     *
     * @ghidraAddress 0x0038fe58
     */
    virtual void OnUnknownSlot36();

    /**
     * Draw the view.
     *
     * Slot 37. Forwards to Rnd::Drawable::Draw() on the Drawable subobject of mUnknown14, at
     * `+0x18` within the view. The name is inferred from the Rnd::Drawable routine it forwards to.
     *
     * @ghidraAddress 0x00390788
     */
    virtual void Draw();

    /**
     * Resolve the three views the container produced and hide the screen.
     *
     * Slot 38. The scene root is the object named by mUnknown80 with `.view` appended. A container
     * with no such object trips the diagnostic `the screen %s doesn't have a valid view!` and
     * leaves mUnknown14 null.
     *
     * @ghidraAddress 0x0038b1b0
     */
    virtual void ResolveContainerViews();

protected:
    /**
     * Act on a message.
     *
     * The override is empty, so a screen that wants messages overrides the slot again.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x003907a8
     */
    virtual void HandleMessage(Message *pMsg);

protected:
    // 0x0038bd60
    // Resolves the two animation views from the screen name and records the enter
    // animation's end frame. Slot 38 and MetTopLogoScreen's slot 38 call it, which is why it is
    // protected. The title is inferred from the two members it writes.
    void ResolveAnimationViews();

private:
    // End frame of the enter animation, read from mUnknown30 by the helper at 0x0038bd60. Not
    // written by the constructor.
    float mUnknown04; // +0x04
    // Time the enter animation started, or zero while no enter animation runs.
    float mUnknown08; // +0x08
    // Time the exit animation started, or zero while no exit animation runs.
    float mUnknown0c; // +0x0c

protected:
    // The renderer this screen registers on. MetLoadFreqBaseScreen reads MetRenderer::mUnknown68
    // through it in slots 5, 19, and 39, which is why it is protected.
    MetRenderer *mUnknown10; // +0x10

    // The container's root view. MetTopLogoScreen's slot 38 resolves it itself, which is why it is
    // protected.
    Rnd::View *mUnknown14; // +0x14

    // MetLoadFreqBaseScreen writes 2 in its slot 30 and 0 in its slot 19, and reads it in its slot
    // 36 to choose between the gizmo panels and the three button actions, which is why it is
    // protected.
    int mUnknown18; // +0x18, starts at 2

private:
    int mUnknown1c;        // +0x1c
    HxStr mUnknown20;      // +0x20, the screen name
    HxStr mUnknown28;      // +0x28, mUnknown80 with `.rnd` appended
    Rnd::View *mUnknown30; // +0x30, the view named `<screen>_EE.anim`
    Rnd::View *mUnknown34; // +0x34, the view named `<screen>_BF.anim`
protected:
    // Extra container object names a screen wants resolved. MetMemCardLoadScreen,
    // MetMemCardTypeScreen, MetRemixTypeScreen, and MetRemixDelScreen each append one in their
    // constructors, which is why it is protected.
    std::vector<HxStr> mUnknown38; // +0x38

private:
    std::list<Rnd::Drawable *> mUnknown44; // +0x44

protected:
    // Set while the container views still need resolving, and cleared by slot 38. Read by
    // MetJukeboxBaseScreen::SetShowing(), which is why it is protected.
    int mUnknown48; // +0x48, starts at 1

public:
    /**
     * Set when the screen was pushed before its container load finished.
     *
     * UpdateFrame() selects its deferred-entry half on this field and clears it once the screen has
     * entered. MetRenderer writes 1 through the screen pointer at `0x0036b28c`, `0x0036b3cc`, and
     * `0x0037164c`, each immediately after dispatching slot 14, so the access rule gives public. A
     * friend declaration for MetRenderer fits the image equally well. +0x4c
     */
    int mUnknown4c;

    /**
     * Set when the screen was activated as a panel before its container load finished.
     *
     * Written by MetRenderer at `0x0036b294`, `0x0036b3d4`, and `0x0037164c` on the same three
     * paths as mUnknown4c, and public for the same reason. +0x50
     */
    int mUnknown50;

private:
    int mUnknown54; // +0x54

public:
    /**
     * Scale on the input auto-repeat interval.
     *
     * The auto-repeat driver at `0x002e55f8` multiplies the field by 50.0f and truncates to an
     * integer, so the initial 1.0f gives a 50 millisecond repeat period. That reader is neither a
     * MetScreen nor a MetRenderer: MetRenderer stores it at its own `+0x94`. A single friend
     * declaration therefore cannot cover this access and the two above, which is what settles the
     * three as public rather than as friendship. +0x58, starts at 1.0f
     */
    float mUnknown58;

protected:
    // Cleared by the MetSaveRemixScreen constructor, which is why it is protected.
    int mUnknown5c; // +0x5c, starts at 1
    // Gates the drawable-list walk in SetShowing(). Cleared by the MetRemixLoadScreen and
    // MetRemixDelScreen constructors, which is why it is protected.
    int mUnknown60; // +0x60, starts at 1

private:
    float mUnknown64; // +0x64
    // Object whose state slots 28 and 29 alternate through 0x00534a48. The exact class is not
    // identified, and Rnd::Object is the base that the destructor of MetButtonList proves for the
    // same class by releasing a vector of them through 0x00520be0.
    Rnd::Object *mUnknown68; // +0x68
    int mUnknown6c;          // +0x6c, not written by the constructor
    int mUnknown70;          // +0x70, not written by the constructor
    float mUnknown74;        // +0x74, not written by the constructor
    int mUnknown78;          // +0x78
    int mUnknown7c;          // +0x7c
    HxStr mUnknown80;        // +0x80, the container name without its suffix
    int mUnknown88;          // +0x88, the load priority
};
