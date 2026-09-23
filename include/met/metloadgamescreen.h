#pragma once

#include "met/fadeuser.h"
#include "met/metfade.h"
#include "met/metscreen.h"

namespace Rnd {
class Text;
} // namespace Rnd

/**
 * Transition screen shown while a game loads.
 *
 * `17MetLoadGameScreen` in the RTTI descriptor at `0x00901f00`, with two public non-virtual bases
 * at fixed offsets, MetScreen at `+0x00`, and FadeUser at `+140`.
 *
 * The 40-entry primary vtable is at `0x007f5f18`, one entry longer than the MetScreen table, so
 * the class declares one virtual of its own, at slot 39.
 *
 * The four-entry FadeUser table at `0x007f5ef0` adjusts `this` by `-140` in every entry.
 *
 * The constructor at `0x0028d2c8` takes only the renderer and the load priority. It supplies the
 * empty string at `0x007f5ca8` for the screen name, `metagame/Transition` for the directory, and
 * `loadgame` for the container. The empty screen name makes the two animation views resolve as
 * `_EE.anim` and `_BF.anim` with nothing before the underscore, which MetMemDetectStartup also
 * does. It zeroes `+0x94` through `+0xa8`, the pair at `+0xa0` in one 8-byte store, and then
 * builds mFade.
 *
 * It declares one virtual of its own at slot 39, at `0x0028e158`, and the name is not recovered.
 *
 * The object is 0xb0 bytes, the size New() requests. The last member, mFade, ends at `+0xac`, and
 * the eight-byte mDeadlineNs gives the class eight-byte alignment.
 *
 * The destructor at `0x00291a50` releases mFade and then runs the MetScreen destructor. MetFade
 * has no destructor of its own, which is why the release is a bare deallocator call with no null
 * test.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x0028d590`, 20 `0x00291620`, 21 `0x00291628`, 22 `0x00291630`, 23 `0x00291638`, 24
 * `0x00291640`, 26 `0x0028dd38`, 33 `0x0028dc60`, 38 `0x0028d4c0`, 39 `0x0028e158`.
 */
class MetLoadGameScreen : public MetScreen, public FadeUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0028d2c8
     */
    MetLoadGameScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00291a50
     */
    virtual ~MetLoadGameScreen();

    /**
     * Produce a load screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The screen.
     * @ghidraAddress 0x002919c8
     */
    static MetScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Show the loading captions and start the enter animation.
     *
     * Slot 5. Outside a jukebox session, `loading.txt` shows `load_loading` and the event text
     * shows `load_demo`, `load_tut`, `load_remix`, or `load_game` for an attract run, the tutorial,
     * jam mode, or anything else. The win sequence is armed for a solo game on its last remaining
     * level, and slot 39 runs. A jukebox session that returns to
     * `MetJukeboxEditPlaylistScreenDone` clears that return and shows `load_loading` and
     * `load_jukebox`, and any other jukebox session blanks both texts.
     *
     * @ghidraAddress 0x0028d590
     */
    virtual void EnterAndShow();

    /**
     * Wait for the music to fade, then load the level and fade in.
     *
     * Slot 26. Once mWaiting is set and the watchdog time passes mDeadlineNs, the common loads
     * start. An attract run fades in at once. Otherwise the level loads for a net, tutorial, or
     * other game, and polling starts. While polling, the fade in starts once the common and level
     * loads both report done. The fade advances on every call.
     *
     * @param flTime The current renderer time.
     * @ghidraAddress 0x0028dd38
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Start fading the music and arm the wait.
     *
     * Slot 33. The music fades over two seconds, and mDeadlineNs is set 2.1 seconds past the
     * watchdog time.
     *
     * @ghidraAddress 0x0028dc60
     */
    virtual void OnUnknownSlot33();

    /**
     * Resolve `event.txt` into mpEvent.
     *
     * Slot 38. Runs MetScreen::ResolveContainerViews() first.
     *
     * @ghidraAddress 0x0028d4c0
     */
    virtual void ResolveContainerViews();

    /**
     * Assign the burn slots of the players about to play.
     *
     * Slot 39, the one virtual this class declares. The game manager's personas take slots in
     * order. An attract run instead picks one prefabricated identity at random, gives it slot 0,
     * and makes it the game manager's only persona. The title is inferred.
     *
     * @ghidraAddress 0x0028e158
     */
    virtual void AssignBurnSlots();

    /**
     * Start the game once the fade out has finished.
     *
     * FadeUser slot. Clears the background scene, queues a GameManagerDoPlaybackMsg for an attract
     * run and a BeginGameLocalMsg otherwise, removes this screen from the renderer, clears
     * MetFrontEndState::mUnknown10, and clears the display to black.
     *
     * @ghidraAddress 0x0028e018
     */
    virtual void OnFadeInDone();

    /**
     * Do nothing.
     *
     * FadeUser slot. The body is empty in the image.
     *
     * @ghidraAddress 0x00291b80
     */
    virtual void OnFadeOutDone();

    /**
     * Silence the slide sound.
     *
     * Every one of the five overrides below is a two-instruction stub, so each was written inline
     * with an empty body. A transition screen plays no navigation sound.
     *
     * @ghidraAddress 0x00291620
     */
    virtual void PlaySlideSound(int) {
    }

    /**
     * Silence the leave sound.
     *
     * @ghidraAddress 0x00291628
     */
    virtual void PlayLeaveSound(int) {
    }

    /**
     * Silence the high sound.
     *
     * @ghidraAddress 0x00291630
     */
    virtual void PlayHighSound(int) {
    }

    /**
     * Silence the cycle-left sound.
     *
     * @ghidraAddress 0x00291638
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * @ghidraAddress 0x00291640
     */
    virtual void PlayCycleRightSound(int) {
    }

private:
    // 0x0028df18
    // Load the level of the net game. The game settings are copied first.
    void LoadNetLevel();

    // 0x00291ad0
    // Record game phase 1 and load the level.
    void LoadGameLevel();

    // 0x00291b28
    // Record the tutorial phase and load the level.
    void LoadTutorialLevel();

    // `event.txt`, which slot 38 resolves. Not written by the constructor. +0x90
    Rnd::Text *mpEvent;
    // Set while slot 26 polls the level load. +0x94
    int mPolling;
    // Set while slot 26 waits for mDeadlineNs. +0x98
    int mWaiting;

public:
    /**
     * Written 1 by MetLogoScreen's slot 36 at `0x002baf20` before it pushes this screen for the
     * attract mode. Public because that write goes through the screen pointer directly. +0x9c
     */
    int mUnknown9c;

private:
    // The watchdog time slot 26 waits for, in nanoseconds. +0xa0
    long long mDeadlineNs;
    // The fade driver, built from the renderer. +0xa8
    MetFade *mFade;
};
