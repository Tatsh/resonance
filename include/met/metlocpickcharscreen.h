#pragma once

#include <vector>

#include "met/metmemdetectscreen.h"

class MetPersonaData;
struct MetScreenCommand;

namespace Rnd {
class Button;
class Mat;
class Mesh;
class Tex;
class View;
} // namespace Rnd

/**
 * Local-multiplayer screen that picks a character.
 *
 * `20MetLocPickCharScreen` in the RTTI descriptor at `0x00901bf0`, with MetMemDetectScreen as its
 * one public non-virtual base at offset 0. New() allocates 0x140 bytes. The 44-entry primary vtable
 * is at `0x007f9a88` and the 21-entry MemcardUser table at `0x007f99d8` adjusts `this` by `-140`.
 * The primary is the same length as the MetMemDetectScreen table, and the class declares no new
 * virtual.
 *
 * Each player cycles through mPersonas with the left and right commands and locks a character in
 * with the select command. The list starts with the personas on every formatted card after a
 * probe, or with the saved personas, and ends with the prefabricated identities of
 * MetFreqMakerAssetManager. When every player has picked, the screen copies the choices into
 * MetFrontEndState, hands them to the game manager, and exits five frames later.
 *
 * The translation unit spans `0x002b1088` to `0x002ba4a0`. Besides the members below, it has the
 * type function at `0x002b9e00`, a per-unit copy of MsgSink::HandleDefault, and template library
 * emissions.
 */
class MetLocPickCharScreen : public MetMemDetectScreen {
public:
    /**
     * Construct the screen.
     *
     * Supplies `mpc` for the screen name, `metagame/_Local` for the directory, and `character_loc`
     * for the container, and records the help text `loc_pc`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002b1270
     */
    MetLocPickCharScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002b19b0
     */
    virtual ~MetLocPickCharScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002b9e60
     */
    static MetLocPickCharScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Show the pickers, first refilling mPersonas when MetLocNumPlayScreen brought the screen up.
     *
     * Slot 5. After MetLocNumPlayScreen, a front end that uses a card runs StartDetect() instead,
     * and one without a card appends the saved personas.
     *
     * @ghidraAddress 0x002b29e8
     */
    virtual void EnterAndShow();

    /**
     * Show the pickers once the probe's last dialogue closes, or defer to the probe.
     *
     * Slot 15.
     *
     * @param name The message screen that was dismissed.
     * @param nChoice The chosen button, counted from zero.
     * @ghidraAddress 0x002ba178
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Cycle, lock in, or release one player's character, or back out.
     *
     * Slot 19. A command from a controller beyond mPlayerCount is ignored.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x002b2370
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the slide sound for a player who has not locked a character in.
     *
     * Slot 20.
     *
     * @param nSelector The controller, counted from 1.
     * @ghidraAddress 0x002b9fe8
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play nothing.
     *
     * Slot 22. The body is empty.
     *
     * @ghidraAddress 0x002b9e58
     */
    virtual void PlayHighSound(int) {
    }

    /**
     * Play the cycle-left sound for a player who has not locked a character in.
     *
     * Slot 23.
     *
     * @param nSelector The controller, counted from 1.
     * @ghidraAddress 0x002b9f48
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play the cycle-right sound for a player who has not locked a character in.
     *
     * Slot 24.
     *
     * @param nSelector The controller, counted from 1.
     * @ghidraAddress 0x002b9f98
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Count down mExitCountdown one step per frame and exit forwards when it reaches zero, then
     * run the base.
     *
     * Slot 26.
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress 0x002ba038
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Bring up the next screen once this one has exited.
     *
     * Slot 36. MetLeftGizmoScreen always comes up. A back command then returns to
     * MetLocNumPlayersScreen, and a finished pick moves on to MetModeScreen.
     *
     * @ghidraAddress 0x002b3e18
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the base views and the four players' materials, views, buttons, meshes, and burn
     * textures, and the two controller icon materials.
     *
     * Slot 38.
     *
     * @ghidraAddress 0x002b1e00
     */
    virtual void ResolveContainerViews();

    /**
     * Show the multi-player detection notice, then run the base probe.
     *
     * Slot 39. The notice receives MetFrontEndState::mUnknown2c.
     *
     * @ghidraAddress 0x002b4068
     */
    virtual void StartDetect();

    /**
     * Skip the base persona load and go straight to StartSaveSpaceCheck().
     *
     * Slot 40. Sets mUnknown98 as the base does.
     *
     * @ghidraAddress 0x002ba148
     */
    virtual void StartLoadPersonas();

    /**
     * Offer to retry the probe or to continue without a card.
     *
     * Slot 41. Raises `mem_check`.
     *
     * @ghidraAddress 0x002b4378
     */
    virtual void OnNoCard();

    /**
     * Load the personas from every formatted card, or append the saved personas and show the
     * pickers when the front end uses no card.
     *
     * Slot 42. With a card but none formatted, the screen closes the message screen and shows the
     * pickers when the dismissal arrives.
     *
     * @ghidraAddress 0x002b4738
     */
    virtual void OnDetectFinished();

    /**
     * Record one card's personas and load the next formatted card, or close the loading notice.
     *
     * MemcardUser slot 13. mCardPersonaStarts receives the size of mPersonas after the card, or
     * -1 when the load failed.
     *
     * @param nPortSlot The card the personas came from. The body does not read it.
     * @param nStatus The result, 0 on success.
     * @ghidraAddress 0x002b4ff0
     */
    virtual void OnPersonasLoaded(int nPortSlot, int nStatus);

private:
    // 0x002b27f8
    // Steps one player's choice through mPersonas and shows the new character. The title is
    // inferred.
    void CyclePersona(const MetScreenCommand *pCommand);

    // 0x002b2e08
    // Points a player's choice at the entry of mPersonas with the persona's username, or at a
    // random entry when there is none. The title is inferred.
    void SelectPersona(MetPersonaData *pPersona, int nPlayer);

    // 0x002b2ef8
    // Loads the layout for the player count, fills mPersonas and the choices, dresses each
    // player's picker, and enters. The title is inferred.
    void ShowPickers();

    // Players who have locked a character in.
    int mReadyCount; // +0xa0
    // From MetFrontEndState::mUnknown2c.
    int mPlayerCount; // +0xa4
    // Each view shows once its player locks a character in.
    std::vector<Rnd::View *> mSelectViews;      // +0xa8
    std::vector<Rnd::Mat *> mCharacterMats;     // +0xb4
    std::vector<Rnd::Button *> mNameButtons;    // +0xc0
    std::vector<Rnd::Mesh *> mControllerMeshes; // +0xcc
    // Each player's index into mPersonas.
    std::vector<int> mChoices;             // +0xd8
    std::vector<Rnd::Tex *> mBurnTextures; // +0xe4
    // Frames until the forward exit.
    float mExitCountdown;                          // +0xf0
    std::vector<MetPersonaData *> mChosenPersonas; // +0xf4
    // The two icon materials are not written by the constructor.
    Rnd::Mat *mDefaultIconMat;               // +0x100
    Rnd::Mat *mCustomIconMat;                // +0x104
    std::vector<MetPersonaData *> mPersonas; // +0x108
    // Indices into GlobalSettings::mCardSlots.
    std::vector<int> mFormattedCards; // +0x114
    // Index into mFormattedCards of the card being loaded.
    int mCardIndex;  // +0x120
    int mUnknown124; // +0x124
    int mUnknown128; // +0x128
    // The two flags are not written by the constructor.
    int mShowOnDismiss;                  // +0x12c
    int mLoadingCards;                   // +0x130
    std::vector<int> mCardPersonaStarts; // +0x134
};
