#pragma once

#include <list>
#include <vector>

#include "app/msgsink.h"
#include "met/metrenderer.h"
#include "os/hxstr.h"
#include "rnd/drawable.h"
#include "rnd/object.h"
#include "rnd/view.h"

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
 *  - 4 `0x00390200` resolves a screen by name, hands it to the renderer routine at `0x003719e0`,
 *    and runs slot 5 on it once slot 14 reports the load finished. Otherwise clears mUnknown4c on
 *    the resolved screen.
 *  - 5 `0x003900a8` runs slot 17 with 1 and then slot 31 with the renderer time.
 *  - 6 `0x0038b828` takes a screen registry key, which every caller supplies as a screen class
 *    name such as `MetConfigControllerScreen`. An empty name clears MetRenderer::mUnknown80.
 *    Otherwise
 *    resolves the screen, and once slot 14 reports the load finished hands it to the renderer
 *    routine at `0x003714c8`, sets MetRenderer::mUnknown80, and runs slot 7 on it.
 *  - 7 `0x0038fdf8` empty.
 *  - 8 `0x003902d0` takes a screen name, writes `Exiting screen: %s` to the memory log, resolves
 *    the screen, and runs slot 9 on it.
 *  - 9 `0x00390100` runs slot 34 with the renderer time.
 *  - 10 `0x00390130` empty.
 *  - 11 `0x00390138` empty.
 *  - 12 `0x0038fe00` empty.
 *  - 13 `0x003900a0` empty.
 *  - 14 `0x0038b338` polls the RndAsyncLoader registered for mUnknown28, runs slot 38 once the
 *    poll succeeds and mUnknown48 is set, and reports whether the screen is ready.
 *  - 15 `0x0038fe20` empty. MetConfigControllerScreen fills it at `0x00206bb0` with a body that
 *    compares an `HxStr` argument against a literal and then runs slot 6, which fixes the
 *    signature as one `const HxStr &` parameter.
 *  - 16 `0x0038fe28` empty.
 *  - 17 `0x0038b490` SetShowing().
 *  - 18 `0x0038aa00` takes a directory and a file name. Appends a separator to the directory,
 *    interns a 12-byte record in the loader map under mUnknown28, builds an RndAsyncLoader for it
 *    with mUnknown88 as the priority, and enqueues the load. The file argument is declared and
 *    ignored.
 *  - 19 `0x0038fe30` empty. MetConfigControllerScreen fills it at `0x00200ba8` with a dispatcher
 *    that reads a selector from `+0x00` of its argument and a sequence number from `+0x04`,
 *    which fixes the signature as one pointer to a command record.
 *  - 20 `0x00390140` PlaySlideSound().
 *  - 21 `0x00390160` PlayLeaveSound().
 *  - 22 `0x003901c0` PlayHighSound().
 *  - 23 `0x00390180` PlayCycleLeftSound().
 *  - 24 `0x003901a0` PlayCycleRightSound().
 *  - 25 `0x003901e0` PlayErrorSound().
 *  - 26 `0x0038fe38` empty.
 *  - 27 `0x0038fe40` empty.
 *  - 28 `0x00390498` takes two floats, an object, and a count. Records the object in mUnknown68
 *    with the two floats in mUnknown64 and mUnknown74, sets mUnknown70 to twice the count, clears
 *    mUnknown6c, and sets the object state through `0x00534a48`.
 *  - 29 `0x003904e0` takes a float. Advances the alternating object state that slot 28 set up, one
 *    step each time the interval in mUnknown74 elapses, until mUnknown6c arrives at mUnknown70.
 *  - 30 `0x0038fe48` empty. MetConfigOptionsButtonsScreen fills it at `0x00207fc0` with a body
 *    that copies the `HxStr` at `+0x04` of its argument, which fixes the signature as one
 *    pointer to a record whose name sits at `+0x04`.
 *  - 31 `0x003905c0` takes a float. Records it in mUnknown08, clears mUnknown0c, and rewinds
 *    mUnknown30 to mUnknown04 through Rnd::Animatable::SetFrame().
 *  - 32 `0x003905f0` takes a float. Drives mUnknown30 from mUnknown08, sets mUnknown7c when the
 *    animation passes its end, and on the following call sets mUnknown1c, clears mUnknown08, and
 *    runs slot 33.
 *  - 33 `0x0038fe50` empty. MetConfigControllerScreen fills it at `0x002069d0` with a body that
 *    reads no argument, which fixes the signature as taking none.
 *  - 34 `0x003906a0` takes a float. Records it in mUnknown0c, clears mUnknown1c and mUnknown08.
 *  - 35 `0x003906b0` takes a float. Drives mUnknown34 from mUnknown0c, and when the animation
 *    passes its end runs slot 17 with 0, hands this screen to the renderer routine at
 *    `0x00371a78`, and runs slot 36.
 *  - 36 `0x0038fe58` empty. MetConfigControllerScreen fills it at `0x00201790` with a body that
 *    reads no argument, which fixes the signature as taking none.
 *  - 37 `0x00390788` Draw().
 *  - 38 `0x0038b1b0` resolves mUnknown30 and mUnknown34 from `%s_EE.anim` and `%s_BF.anim`
 *    through the helper at `0x0038bd60`, resolves mUnknown14 by appending `.view` to mUnknown80,
 *    runs slot 17 with 0, and clears mUnknown48.
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
     * @ghidraAddress 0x00390140
     */
    virtual void PlaySlideSound();

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
     * @ghidraAddress 0x003901c0
     */
    virtual void PlayHighSound();

    /**
     * Play the sound that accompanies cycling a value to the left.
     *
     * Passes the literal `SND_MET_CYCLE_L` to the named-sound player at `0x0012f470`.
     *
     * @ghidraAddress 0x00390180
     */
    virtual void PlayCycleLeftSound();

    /**
     * Play the sound that accompanies cycling a value to the right.
     *
     * Passes the literal `SND_MET_CYCLE_R` to the named-sound player at `0x0012f470`.
     *
     * @ghidraAddress 0x003901a0
     */
    virtual void PlayCycleRightSound();

    /**
     * Play the sound that accompanies a rejected input.
     *
     * Passes the literal `SND_MET_ERROR` to the named-sound player at `0x0012f470`. This is the
     * one sound of the six that MetScreenMultiSoundBank does not override.
     *
     * @ghidraAddress 0x003901e0
     */
    virtual void PlayErrorSound();

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
