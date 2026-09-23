#pragma once

#include <vector>

#include "met/metscreenmultisoundbank.h"

namespace Rnd {
class Text;
} // namespace Rnd

/**
 * Base of the four pause screens.
 *
 * `18MetPauseBaseScreen` in the RTTI descriptor at `0x00902300`, with MetScreenMultiSoundBank as
 * its one public non-virtual base at offset 0. The vtable is at `0x00802be8` and has 40 entries,
 * one more than the MetScreen table, so the class declares exactly one virtual of its own at slot
 * 39. New() allocates 0xb0 bytes.
 *
 * Four classes derive from the class, MetPauseGameScreen, MetPauseMultiRemixScreen,
 * MetPauseSoloGameScreen, and MetPauseSoloRemixScreen. Each passes its own directory and container
 * to the constructor and then records its class name in mUnknown9c, the panel a dismissed
 * confirmation reactivates. A derived slot 38 resolves the option texts into mUnknowna4, and a
 * derived slot 5 fills mUnknown90 with their labels before this class's slot 5 copies them across.
 *
 * A command records the exit action in mUnknown8c and begins the exit. Slot 36 runs when the exit
 * finishes and either asks for confirmation of a quit or a restart or unpauses the game. Slot 15
 * acts on the confirmation.
 *
 * Slots 20 through 24 are two-instruction `jr ra` stubs, so a pause screen plays none of the five
 * sounds its base swapped for the multiplayer bank. Slot 39 plays the bank's slide sound instead.
 */
class MetPauseBaseScreen : public MetScreenMultiSoundBank {
public:
    /**
     * Construct the screen with an empty option list.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @param name The screen name.
     * @param directory The directory the container loads from.
     * @param file The container name.
     * @ghidraAddress 0x00317d40
     */
    MetPauseBaseScreen(MetRenderer *pRenderer,
                       int nPriority,
                       const HxStr &name,
                       const HxStr &directory,
                       const HxStr &file);

    /**
     * @ghidraAddress 0x00317e90
     */
    virtual ~MetPauseBaseScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @param name The screen name.
     * @param directory The directory the container loads from.
     * @param file The container name.
     * @return The new screen.
     * @ghidraAddress 0x0031c038
     */
    static MetPauseBaseScreen *New(MetRenderer *pRenderer,
                                   int nPriority,
                                   const HxStr &name,
                                   const HxStr &directory,
                                   const HxStr &file);

    /**
     * Copy each label in mUnknown90 to the option text at the same index, then enter.
     *
     * Slot 5.
     *
     * @ghidraAddress 0x00318278
     */
    virtual void EnterAndShow();

    /**
     * Act on the quit or restart confirmation.
     *
     * Slot 15. The first button reactivates the pause panel. The second queues an
     * UnpauseGameSystemMsg and then either records phase 4 in MetFrontEndState and runs
     * GrooveWorld::PostExitMode2() for a quit, or runs GrooveWorld::PostExitMode3() for a restart.
     *
     * @param name The confirmation that was dismissed.
     * @param nChoice The button chosen.
     * @ghidraAddress 0x00318b80
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Record the exit action for a command and begin the exit.
     *
     * Slot 19. Code 10 resumes and a back asks to quit. A select asks to restart, but only in game
     * mode outside a net game, or in front-end phase 5. Each plays the bank's slide sound and
     * clears the active panel first.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x00318010
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0031bff0
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * @param nSelector The pad index of the command, which the body does not read.
     * @ghidraAddress 0x0031bff8
     */
    virtual void PlayLeaveSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0031c000
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0031c008
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0031c010
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Act on the recorded exit action once the exit finishes.
     *
     * Slot 36. A quit or a restart shows a two-button `NO`/`YES` confirmation through
     * MetMsgScreen::Show() with this screen as the owner. Any other action queues an
     * UnpauseGameSystemMsg.
     *
     * @ghidraAddress 0x00318418
     */
    virtual void OnUnknownSlot36();

    /**
     * Play the multiplayer bank's slide sound, which slot 20 no longer plays.
     *
     * Slot 39, the one virtual this class declares. The name is inferred.
     *
     * @param nSelector The pad index of the command.
     * @ghidraAddress 0x0031c018
     */
    virtual void PlayPauseSound(int nSelector);

protected:
    /** The exit action HandleCommand() recorded. +0x8c */
    enum ExitAction {
        kExitNone = 0,        /*!< No command has arrived. */
        kExitResume = 1,      /*!< Unpause the game. */
        kExitQuit = 3,        /*!< Confirm, then quit the game. */
        kExitRestart = 4,     /*!< Confirm, then restart the game. */
        kExitGameOptions = 5, /*!< Open the game options, recorded by the solo screens only. */
        kExitController = 6,  /*!< Open the controller set-up, recorded by the solo screens only. */
    };

    int mUnknown8c;                      // +0x8c, an ExitAction
    std::vector<HxStr> mUnknown90;       // The option labels a derived slot 5 fills.
    HxStr mUnknown9c;                    // The panel a dismissed confirmation reactivates.
    std::vector<Rnd::Text *> mUnknowna4; // The option texts a derived slot 38 resolves.
};
