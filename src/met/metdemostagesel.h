#pragma once

#ifdef VIDEO_STANDARD_PAL
#include "met/metbuttonlist.h"
#include "met/metscreen.h"
#include "met/texturepairrecord.h"
#include "rnd/mesh.h"
#include "rnd/text.h"

/**
 * Song picker of the European release's demo, with a tutorial button and two or four song buttons.
 *
 * The European release added the class. Its RTTI descriptor is at `0x009357d0`, and MetScreen is
 * its one public non-virtual base at offset 0. The 39-entry primary vtable at `0x0082fc78` has the
 * length of the MetScreen table, and the class declares no new virtual.
 *
 * The constructor supplies `ss` for the screen name, `metagame/demo` for the directory, and
 * `stage_sel_demo` for the container. The 0xb0-byte allocation in New() fixes the size. Neither
 * New() nor the constructor has a caller in the image. MetScreen::CreateFrontEndScreens() does not
 * register the screen, and the class is unreachable in the European release.
 *
 * In remix mode the screen shows `stage_3buts.view` with `ss_stage1.but` through `ss_stage3.but`,
 * and otherwise `stage_5buts.view` with `ss_stage1.but` through `ss_stage5.but`. The first button
 * starts the tutorial, and each other button starts one demo song. A button's prompt is the
 * MetScreen::mHelpKeys entry at its index.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5, 9, 19, 23, 24, 26, 30, 33, 36, and 38, all declared below.
 */
class MetDEMOStageSel : public MetScreen {
public:
    /**
     * Construct the screen with every member cleared.
     *
     * ResolveContainerViews() creates the texture pairs and the button list.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress PAL: 0x00221fc0
     */
    MetDEMOStageSel(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the two texture pairs and the button list.
     *
     * @ghidraAddress PAL: 0x0022a328
     */
    virtual ~MetDEMOStageSel();

    /**
     * Allocate and construct the screen.
     *
     * The 0xb0-byte allocation is billed to the tag `MsgSink`. The routine has no caller.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress PAL: 0x0022a2a0
     */
    static MetDEMOStageSel *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Show the screen, then build the buttons, prompts, and title for the play mode. Slot 5.
     *
     * The MetScreen body runs first. Both texture pairs are invalidated, both television meshes
     * hide, and MetScreen::mHelpKeys is emptied. Remix mode swaps `stage_3buts.view` into
     * `stage_buts.view`, adds three buttons, and selects the second. It then pushes the prompts
     * `learn to play remix mode`, `remix the first song`, and `remix the second song`, labels the
     * buttons from the remix button texts, and sets the remix title. The game mode does the same
     * with `stage_5buts.view`, five buttons, the four game prompts, and the game title. The
     * selected song is shown last.
     *
     * @ghidraAddress PAL: 0x002221b8
     */
    virtual void EnterAndShow();

    /**
     * Start the exit and depart the companion screens. Slot 9.
     *
     * The MetScreen body runs first. `MetScreenTitleScreen` always exits, and `MetHelpScreen` exits
     * as well when MetScreen::mExitChoice is 2.
     *
     * @ghidraAddress PAL: 0x00223f40
     */
    virtual void BeginExit();

    /**
     * Move between buttons, start the selection, or depart. Slot 19.
     *
     * A previous or next command steps mStageList, shows the new button's prompt, and shows the
     * selected song. A select command on the first button sets kGameModeSolo and records the
     * tutorial level with the language suffix, the first arena, and the easy difficulty. A select
     * command on another button records the button's demo level, clears
     * GameParams::mLoadingGame, takes the first arena for the first two songs and the third for the
     * last two, and selects the normal difficulty for the last two. It runs the selection script
     * template. Either selection records this screen or `MetTutorialScreen` in
     * MetFrontEndState::mReturnScreen, sets MetScreen::mExitChoice to 2, and exits. A back command
     * sets MetScreen::mExitChoice to 0 and exits. Every other command is discarded.
     *
     * @param pCommand The command to handle.
     * @ghidraAddress PAL: 0x00224558
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Silence the cycle-left sound. Slot 23.
     *
     * @ghidraAddress PAL: 0x0022a290
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound. Slot 24.
     *
     * @ghidraAddress PAL: 0x0022a298
     */
    virtual void PlayCycleRightSound(int) {
    }

    /**
     * Show each television texture once its load has finished. Slot 26.
     *
     * Each pair that advances hides its mesh and, when the pair has a current texture, shows the
     * mesh with that texture. mLoadPending clears once both meshes show a texture in the same
     * frame.
     *
     * @param flTime The renderer's current frame. The body does not read it.
     * @ghidraAddress PAL: 0x00224428
     */
    virtual void UpdateIdle(float flTime);

    /**
     * Ignore the end of a button alternation. Slot 30.
     *
     * @param pButton The button. The body does not read it.
     * @ghidraAddress PAL: 0x0022a400
     */
    virtual void OnRepeatingSoundFinished(Rnd::Button *pButton);

    /**
     * Show the selected button's prompt. Slot 33.
     *
     * @ghidraAddress PAL: 0x0022a3c8
     */
    virtual void OnEnterFinished();

    /**
     * Push the next screens once the exit finishes. Slot 36.
     *
     * A back exit pushes `MetScreenTitleScreen`, `MetLeftGizmoScreen`, and `MetModeScreen` and
     * activates `MetModeScreen`. A selection pushes and activates `MetLoadGameScreen`.
     *
     * @ghidraAddress PAL: 0x002240b0
     */
    virtual void OnExitFinished();

    /**
     * Create the texture pairs and the button list, and resolve the container objects. Slot 38.
     *
     * Every run allocates new texture pairs and a new button list without deleting the previous
     * ones.
     *
     * @ghidraAddress PAL: 0x002238c8
     */
    virtual void ResolveContainerViews();

private:
    // PAL: 0x00224ec0
    // Starts the selected song's logo and picture loads and fills the three level texts.
    void ShowSelectedSong();

    TexturePairRecord *mLogoPair;  // +0x8c `gSongLogo1.tex` and `gSongLogo2.tex`
    TexturePairRecord *mLabelPair; // +0x90 `gSongLabel1.tex` and `gSongLabel2.tex`
    // Set by ShowSelectedSong() and cleared by slot 26 once both textures show. +0x94
    int mLoadPending;
    MetButtonList *mStageList; // +0x98 The tutorial button and the song buttons.
    Rnd::Text *mGenreText;     // +0x9c `ss_genre_bpm.txt`
    Rnd::Mesh *mTvLogoMesh;    // +0xa0 `sstv_logo right.mesh`
    Rnd::Mesh *mTvLabelMesh;   // +0xa4 `sstv_label_right.mesh`
    Rnd::Text *mBioText;       // +0xa8 `ss_bio.txt`
    Rnd::Text *mLabelText;     // +0xac `ss_label.txt`
};
#endif
