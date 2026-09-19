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
 * Base of every front-end screen.
 *
 * `9MetScreen` in the RTTI descriptor at `0x008eed98`, with MsgSink as its one public non-virtual
 * base at offset 0. The object is 0x8c bytes, which MetMemDetectScreen, MetSaveRemix,
 * MetSoloWinScreen, and seven further classes fix by placing their second base at `+140`. The
 * vtable is at `0x0080b6a0` and has 39 entries.
 *
 * Thirty-two classes derive directly from the class, and five intermediate classes sit between it
 * and their own children, MetGizmoPanel, MetJukeboxBaseScreen, MetMultiTipsBaseScreen,
 * MetPauseBaseScreen, and MetScreenMultiSoundBank.
 *
 * A screen owns a Rnd::View loaded from a `.rnd` container. The constructor records the container
 * name and, when both name arguments are non-empty, starts the load through vtable slot 18. Slot
 * 14 polls the RndAsyncLoader until the load completes and then runs slot 38, which resolves the
 * view by appending `.view` to the container name and resolves the two animation views by
 * formatting `%s_EE.anim` and `%s_BF.anim` from the screen name. A screen with no view trips the
 * diagnostic `the screen %s doesn't have a valid view!`.
 *
 * Every entry of the 39-entry table, with the slots that this class does not name recorded by
 * address and behaviour. Slot 0 is the compiler-generated GetTypeInfo at `0x0038fd78` and is not
 * source. An entry titled empty is a two-instruction `jr ra` stub, which is an inline virtual with
 * an empty body.
 *
 *  - 1 `0x0038a848` the destructor.
 *  - 2 `0x00105158` MsgSink::Handle(), inherited unchanged.
 *  - 3 `0x003907a8` MsgSink::HandleMessage(), overridden empty.
 *  - 4 `0x00390200` PushNamedScreen().
 *  - 5 `0x003900a8` EnterAndShow().
 *  - 6 `0x0038b828` ActivateNamedPanel().
 *  - 7 `0x0038fdf8` empty.
 *  - 8 `0x003902d0` ExitScreenByName().
 *  - 9 `0x00390100` BeginExit().
 *  - 10 `0x00390130` empty.
 *  - 11 `0x00390138` empty.
 *  - 12 `0x0038fe00` empty.
 *  - 13 `0x003900a0` empty.
 *  - 14 `0x0038b338` PollContainerLoad().
 *  - 15 `0x0038fe20` empty. MetConfigControllerScreen fills it at `0x00206bb0` with a body that
 *    compares an `HxStr` argument against a literal and then runs slot 6, which fixes the
 *    signature as one `const HxStr &` parameter.
 *  - 16 `0x0038fe28` empty.
 *  - 17 `0x0038b490` SetShowing().
 *  - 18 `0x0038aa00` BeginContainerLoad().
 *  - 19 `0x0038fe30` empty. MetConfigControllerScreen fills it at `0x00200ba8` with a dispatcher
 *    that reads a selector from `+0x00` of its argument and a sequence number from `+0x04`,
 *    which fixes the signature as one pointer to a command record.
 *  - 20 `0x00390140` PlaySlideSound().
 *  - 21 `0x00390160` PlayLeaveSound().
 *  - 22 `0x003901c0` PlayHighSound().
 *  - 23 `0x00390180` PlayCycleLeftSound().
 *  - 24 `0x003901a0` PlayCycleRightSound().
 *  - 25 `0x003901e0` PlayErrorSound().
 *
 * Five of those six sound slots take one integer argument, which an earlier reading recorded as no
 * argument at all. MetKeyboardScreen::StartRepeatingSound() at `0x0028c5e0` loads its own selector
 * into `a1` immediately before each of its five virtual calls, to slots 20, 22, 23, 24, and 25, so
 * the argument is set at a call site rather than inherited from a register. The same class
 * overrides four of the five and each override reads `a1` and compares it against the selector it
 * recorded. Slot 21 stays argument-free. It is the one slot of the six MetKeyboardScreen does not
 * override, nothing loads `a1` before a call to it, and the MetFreqMakerInventoryScreen override at
 * `0x00272bb8` forwards to this class with no argument. The asymmetry is recorded rather than
 * smoothed over.
 *
 * What the selector identifies is not recovered. A screen that records -1 plays its sound for every
 * value, which is the only part of the meaning the image settles.
 *  - 26 `0x0038fe38` empty.
 *  - 27 `0x0038fe40` empty.
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
 * Two data members are protected and the rest are private. MetRemixLoadScreen and
 * MetRemixDelScreen both clear mUnknown60 in their constructors, and MetSaveRemixScreen clears
 * mUnknown5c in its own, so those two are written by derived code and the others are not. Nothing
 * outside the class and its children reads any member.
 */
class MetScreen : public MsgSink {
public:
    /**
     * Construct a screen and start its container load.
     *
     * The load starts only when both pszDirectory and pszFile are non-empty, and it runs through
     * vtable slot 18 rather than directly. Registration as a sink on pRenderer happens last.
     *
     * mUnknown04, mUnknown70, and mUnknown74 are not written, so a screen starts with three
     * indeterminate fields. Slot 38 writes mUnknown04 and slot 28 writes the other two.
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
     * @param flStartTime The time the first step runs at.
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
     * @param flTime The current renderer time.
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
     * Unrecovered. Slot 26.
     *
     * The body is empty here. MetKeyboardScreen fills it at `0x0028c708` with a body that reads its
     * argument out of `f12` and compares a float member against it, which is what fixes the single
     * parameter as a float. Nine further screens fill the slot too. The argument is the renderer
     * time, on the same evidence that fixes it for the two animation slots, because the
     * MetKeyboardScreen body adds a fixed 240 to a recorded value and tests the sum against it.
     *
     * @param flTime The current renderer time.
     * @ghidraAddress 0x0038fe38
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Rewind the enter animation and record the time it starts at.
     *
     * Slot 31. The exit animation start time is cleared, so the two animations never run together.
     *
     * @param flTime The time the animation starts at.
     * @ghidraAddress 0x003905c0
     */
    virtual void StartEnterAnimation(float flTime);

    /**
     * Drive the enter animation and finish it once it passes its end.
     *
     * Slot 32. The end is detected on one call and acted on the next, which is what mUnknown7c
     * records between the two.
     *
     * @param flTime The current renderer time.
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
     * @param flTime The time the animation starts at.
     * @ghidraAddress 0x003906a0
     */
    virtual void StartExitAnimation(float flTime);

    /**
     * Drive the exit animation and hide the screen once it passes its end.
     *
     * Slot 35. The end is detected on one call and acted on the next, through mUnknown78. The
     * screen is hidden, erased from the renderer's stack, and then slot 36 runs.
     *
     * @param flTime The current renderer time.
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
     * Resolve the three views the container produced and hide the screen.
     *
     * Slot 38. The scene root is the object named by mUnknown80 with `.view` appended. A container
     * with no such object trips the diagnostic `the screen %s doesn't have a valid view!` and
     * leaves mUnknown14 null.
     *
     * @ghidraAddress 0x0038b1b0
     */
    virtual void ResolveContainerViews();

    /**
     * Show or hide the view and every drawable the container loaded.
     *
     * Forwards to Rnd::Drawable::SetShowing() on mUnknown14 and then, when mUnknown60 is set, on
     * each entry of the drawable list that the loader recorded for mUnknown28. The name is
     * inferred from the Rnd::Drawable virtual it forwards to.
     *
     * @param nShowing Non-zero to draw the screen.
     * @ghidraAddress 0x0038b490
     */
    virtual void SetShowing(int nShowing);

    /**
     * Play the sound that accompanies sliding between screens.
     *
     * Passes the literal `SND_MET_SLIDE` to the named-sound player at `0x0012f470`. The name is
     * inferred from that literal. MetScreenMultiSoundBank, MetMultiTipsBaseScreen,
     * MetPauseBaseScreen, and MetJukeboxBaseScreen all override this slot, the last three with an
     * empty body, which is what proves the return type is void.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00390140
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play the sound that accompanies departing a screen.
     *
     * Passes the literal `SND_MET_LEAVE` to the named-sound player at `0x0012f470`.
     *
     * @ghidraAddress 0x00390160
     */
    virtual void PlayLeaveSound();

    /**
     * Play the sound that accompanies the emphasised selection.
     *
     * Passes the literal `SND_MET_HIGH` to the named-sound player at `0x0012f470`.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x003901c0
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * Play the sound that accompanies cycling a value to the left.
     *
     * Passes the literal `SND_MET_CYCLE_L` to the named-sound player at `0x0012f470`.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00390180
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play the sound that accompanies cycling a value to the right.
     *
     * Passes the literal `SND_MET_CYCLE_R` to the named-sound player at `0x0012f470`.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x003901a0
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Play the sound that accompanies a rejected input.
     *
     * Passes the literal `SND_MET_ERROR` to the named-sound player at `0x0012f470`. This is the
     * one sound of the six that MetScreenMultiSoundBank does not override.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x003901e0
     */
    virtual void PlayErrorSound(int nSelector);

    /**
     * Draw the view.
     *
     * Forwards to Rnd::Drawable::Draw() on the Drawable subobject of mUnknown14, at `+0x18` within
     * the view. The name is inferred from the Rnd::Drawable routine it forwards to.
     *
     * @ghidraAddress 0x00390788
     */
    virtual void Draw();

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

private:
    // 0x0038bd60. Resolves the two animation views from the screen name and records the enter
    // animation's end frame. Slot 38 is its one caller, and the title is inferred from the two
    // members it writes.
    void ResolveAnimationViews();

    // End frame of the enter animation, read from mUnknown30 by the helper at 0x0038bd60. Not
    // written by the constructor.
    float mUnknown04; // +0x04
    // Time the enter animation started, or zero while no enter animation runs.
    float mUnknown08; // +0x08
    // Time the exit animation started, or zero while no exit animation runs.
    float mUnknown0c;                      // +0x0c
    MetRenderer *mUnknown10;               // +0x10
    Rnd::View *mUnknown14;                 // +0x14
    int mUnknown18;                        // +0x18, starts at 2
    int mUnknown1c;                        // +0x1c
    HxStr mUnknown20;                      // +0x20, the screen name
    HxStr mUnknown28;                      // +0x28, mUnknown80 with `.rnd` appended
    Rnd::View *mUnknown30;                 // +0x30, the view named `<screen>_EE.anim`
    Rnd::View *mUnknown34;                 // +0x34, the view named `<screen>_BF.anim`
    std::vector<HxStr> mUnknown38;         // +0x38
    std::list<Rnd::Drawable *> mUnknown44; // +0x44
    int mUnknown48;                        // +0x48, starts at 1
    int mUnknown4c;                        // +0x4c
    int mUnknown50;                        // +0x50
    int mUnknown54;                        // +0x54
    float mUnknown58;                      // +0x58, starts at 1.0f

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
