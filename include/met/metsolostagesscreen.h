#pragma once

#include <vector>

#include "met/metscreen.h"
#include "met/metsonglists.h"
#include "met/texturepairrecord.h"

class CampaignStats;
class MetButtonList;

namespace Rnd {
class Button;
class Font;
class Mat;
class Mesh;
class Tex;
class Text;
class TransAnim;
class View;
} // namespace Rnd

/**
 * Screen that picks a solo stage and a level within it.
 *
 * `19MetSoloStagesScreen` in the RTTI descriptor at `0x008f0070`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x0080d910`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor takes only the renderer and the load priority, and supplies `ss` for the screen
 * name, `metagame/_Solo` for the directory, and `stage_sel` for the container. The factory's
 * 0x25c-byte allocation fixes the size.
 *
 * Six stage buttons run along mStageList, the sixth being the custom stage. The seven indicator
 * buttons of mIndicatorList pick a level within the selected stage from mCurrentLevels, and the
 * two arrow buttons beside the television scroll the indicator ring. A scroll slides the television
 * panel out and back over 100 frames while the next level's logo and label textures load.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5, 9, 19, 20, 23, 24, 26, 30, 33, 36, and 38, all declared below.
 */
class MetSoloStagesScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * Allocates the two button lists, builds the logo and label texture pairs from
     * `gSongLogo1.tex` with `gSongLogo2.tex` and `gSongLabel1.tex` with `gSongLabel2.tex`, and
     * appends the prompt key `levels` to mUnknown38.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0039e308
     */
    MetSoloStagesScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the two button lists and empty the level lists.
     *
     * @ghidraAddress 0x0039ef80
     */
    virtual ~MetSoloStagesScreen();

    /**
     * Allocate and construct the screen.
     *
     * The 0x25c-byte allocation is billed to the tag `MsgSink`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x003aeb28
     */
    static MetSoloStagesScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Rebuild the stages, caption the title, and show the screen. Slot 5.
     *
     * The screen hides first. When MetFrontEndState's `+0x18` and `+0x10` flags are both set,
     * `+0x10` is cleared, MetGlobalSettingsSaverScreen::StartSave() runs with this screen as the
     * one to return to, mUnknown50 is cleared, and nothing else runs.
     *
     * Otherwise the level lists and the stage buttons are rebuilt with MetFrontEndState's `+0x14`
     * flag as the unlock-everything switch. A set `+0x18` flag moves to `+0x1c`, selects the help
     * layout `standard_title`, pushes `MetHelpScreen`, and resets the game parameters' loading
     * flag. The title reads, from configuration code 0x269, `solo` or `multi`, then `game` with
     * the difficulty name or `remix`, then `stages`. The stage-button view takes
     * `stage_6buts.view` in a game and `stage_5buts.view` in a remix, and the arrow buttons show
     * only beside stages available at the difficulty.
     *
     * @ghidraAddress 0x003a5810
     */
    virtual void EnterAndShow();

    /**
     * Blank the television and exit. Slot 9.
     *
     * @ghidraAddress 0x003a6690
     */
    virtual void BeginExit();

    /**
     * Move between stages, scroll the levels, or act on the selection. Slot 19.
     *
     * A previous or next command steps mStageList and refreshes the stage. A left or right command
     * on an available stage starts the scroll and alternates the arrow button once at 15-frame
     * intervals. A select command on a selectable stage records the level in the game parameters,
     * unless the custom stage is selected, and alternates the stage button twice at 30-frame
     * intervals. A back command exits with `MetScreenTitleScreen`.
     *
     * @param pCommand The command to handle.
     * @ghidraAddress 0x003a4810
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the slide sound when the selected stage is selectable. Slot 20.
     *
     * @param nSelector The selector MetScreen::PlaySlideSound() receives.
     * @ghidraAddress 0x003aed40
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play the cycle-left sound when the selected stage is available. Slot 23.
     *
     * @param nSelector The selector MetScreen::PlayCycleLeftSound() receives.
     * @ghidraAddress 0x003aedb0
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play the cycle-right sound when the selected stage is available. Slot 24.
     *
     * @param nSelector The selector MetScreen::PlayCycleRightSound() receives.
     * @ghidraAddress 0x003aee00
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Swap in loaded textures and run the television scroll. Slot 26.
     *
     * Both texture pairs advance, and the two right-hand television meshes show only while their
     * pair has a current texture. A finished load either fills the television at once or starts
     * the scroll mScrollDirection records. A running scroll moves the panel animation over 100
     * frames and then shows the new level.
     *
     * @param flTime The renderer's current frame.
     * @ghidraAddress 0x003a4df8
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Finish a scroll or a selection once its button stops alternating. Slot 30.
     *
     * An arrow button moves mIndicatorList one step, records the level for the selected stage,
     * and loads its textures. Any other button exits towards the next screen when the selected
     * stage is open, and otherwise reactivates this screen.
     *
     * @param pButton The button whose alternation finished.
     * @ghidraAddress 0x003a54b8
     */
    virtual void OnUnknownSlot30(Rnd::Button *pButton);

    /**
     * Show the television and load the selected level's textures. Slot 33.
     *
     * @ghidraAddress 0x003aee50
     */
    virtual void OnUnknownSlot33();

    /**
     * Push the next screens once the exit finishes. Slot 36.
     *
     * A back exit returns to `MetGameSkillScreen` in a game and to `MetRemixTypeScreen` in a
     * remix. The custom stage lists the remixes on the memory card and the disc. Any other stage
     * goes to `MetLoadGameScreen` with the last arena when GameManagerImpl::GetPersonas() lists
     * three or more personas, and to `MetArenasScreen` otherwise.
     *
     * @ghidraAddress 0x003a6738
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the container objects. Slot 38.
     *
     * @ghidraAddress 0x0039f720
     */
    virtual void ResolveContainerViews();

    /**
     * Materials and fonts one stage-button state uses.
     */
    struct ButtonStyle {
        Rnd::Mat *mMat;   /*!< The material the state shows. +0x00 */
        Rnd::Font *mFont; /*!< The font the state shows. +0x04 */
    };

    /**
     * Status panel material and the mesh that shows it.
     */
    struct StatusSlot {
        Rnd::Mat *mMat;   /*!< The material the level state texture goes to. +0x00 */
        Rnd::Mesh *mMesh; /*!< The mesh the material is drawn on. +0x04 */
    };

private:
    // 0x003a09d0
    // Builds both button lists, the wire meshes, the icon materials, and the stage-button styles.
    void BuildButtons();

    // 0x003a1c88
    // Empties the five stage level lists and the custom list, one element at a time.
    void ClearLevelLists();

    // 0x003a1f38
    // Points mCurrentLevels at the selected stage's list and shows its indicators and wires.
    void RefreshStage();

    // 0x003a2190
    // Reports whether the stage is unavailable in a remix or at the current difficulty.
    int IsStageUnavailable(int nStage);

    // 0x003a2368
    // Shows the stage bonus and the score still needed to beat the stage.
    void ShowStageBonus(int nStage);

    // 0x003a26c0
    // Styles every stage button as open or closed.
    void ApplyStageStyles();

    // 0x003a2878
    // Reports whether a level of the selected stage is locked.
    int IsLevelLocked(int nLevel);

    // 0x003a2d58
    // Styles each indicator button as won, open, or locked.
    void RefreshIndicators(int bStageLocked);

    // 0x003a3238
    // Shows or hides the level texts and the two status meshes.
    void ShowLevelTexts(int nShowing);

    // 0x003a3310
    // Shows the stage warning in place of the television, or the reverse.
    void ShowWarning(bool bShow);

    // 0x003a3510
    // Colours the television and the level texts for a locked, open, or won level.
    void StyleLevel(int bStageLocked, const HxStr &levelName);

    // 0x003a3c68
    // Fills the level texts for the selected level.
    void ShowLevelDetails();

    // 0x003a4438
    // Moves the highlight to the selected stage's arrow buttons.
    void UpdateArrows();

    // 0x003a4640
    // Starts the logo and label loads for the selected level.
    void LoadLevelTextures();

    // 0x003a52f8
    // Starts a scroll of the television panel.
    void StartScroll();

    // 0x003a75a8
    // Copies the level lists from MetSongLists, adding the stage 6 list to the fifth stage.
    void RebuildLevelLists();

    // 0x003a79f0
    // Records which of the two secret levels are open.
    void UpdateSecretLevels(int bUnlockAll);

    // 0x003a7f38
    // Opens or closes each stage and selects the first stage and level still to play.
    void SetUpStages(int bUnlockAll);

    // 0x003aebb0
    // Reports whether a persona has beaten a stage, at the difficulty in a game and at any
    // difficulty otherwise. IsLevelLocked() expands it inline, and this out-of-line copy has no
    // caller.
    int IsStageBeaten(CampaignStats &stats, int nDifficulty, int nStage);

    // 0x003aec70
    // Shows or hides every indicator button.
    void ShowIndicators(int nShowing);

    // 0x003aed00
    // Reports whether a stage and its recorded level are open. Slots 19 and 20 expand it inline,
    // and this out-of-line copy has no caller.
    int IsStageSelectable(int nStage);

    Rnd::Button *mLeftArrow;             // +0x8c "ss_left_0%d.but" for the selected stage
    Rnd::Button *mRightArrow;            // +0x90 "ss_right_0%d.but" for the selected stage
    Rnd::Text *mLabelText;               // +0x94 "ss_label.txt"
    Rnd::Text *mBioText;                 // +0x98 "ss_bio.txt"
    Rnd::Text *mScoreText;               // +0x9c "ss_score.txt"
    Rnd::Text *mGenreText;               // +0xa0 "ss_genre_bpm.txt"
    Rnd::Text *mWarningText;             // +0xa4 "ss_warning.txt"
    Rnd::Text *mStageBonusText;          // +0xa8 "ss_stage_bonus_val.txt"
    Rnd::Text *mStageBeatText;           // +0xac "ss_stage_beat_val.txt"
    Rnd::View *mStageBonusGroup;         // +0xb0 "stage_bonus_group.view"
    Rnd::View *mTvView;                  // +0xb4 "sstv.view"
    Rnd::TransAnim *mTvPanelAnim;        // +0xb8 "stage_sel_tv_panel.tnm"
    std::vector<Rnd::Mat *> mTvMats;     // +0xbc The left and right television screens.
    std::vector<Rnd::Mat *> mTvLogoMats; // +0xc8 The left and right television logos.
    float mScrollLeftTime;               // +0xd4 The frame a left scroll started, or zero.
    float mScrollRightTime;              // +0xd8 The frame a right scroll started, or zero.
    Rnd::Tex *mTvLabelTex;               // +0xdc The label the television shows.
    Rnd::Tex *mNextTvLabelTex;           // +0xe0 The label a scroll brings in.
    // Cleared by slot 38 and never written again, so every reader shows no texture. +0xe4
    Rnd::Tex *mBlankTex;
    Rnd::Tex *mTvLogoTex;     // +0xe8 The logo the television shows.
    Rnd::Tex *mNextTvLogoTex; // +0xec The logo a scroll brings in.
    int mUnknownf0;           // +0xf0 Cleared by slot 38, with no reader.
    int mUnknownf4;           // +0xf4 Cleared by slot 38, with no reader.
    // The status textures, locked, open, and won, from the three `lvlstate_*.bmp` bitmaps. +0xf8
    std::vector<Rnd::Tex *> mLevelStateTexs;
    std::vector<StatusSlot> mStatusSlots; // +0x104 The left and right status panels.
    // 0 while idle, and 1 or 2 while a left or right scroll waits for its textures. +0x110
    int mScrollDirection;
    std::vector<Rnd::Mat *> mWonMats;            // +0x114 Indicator materials for a won level.
    std::vector<Rnd::Mat *> mOpenMats;           // +0x120 Indicator materials for an open level.
    std::vector<Rnd::Mat *> mLockedMats;         // +0x12c Indicator materials for a locked level.
    MetButtonList *mIndicatorList;               // +0x138 "ss_ind_0%d.but", seven buttons
    Rnd::View *mStageButtonsView;                // +0x13c "stage_buts.view"
    MetButtonList *mStageList;                   // +0x140 "ss_stage%d.but", six buttons
    std::vector<ButtonStyle> mOpenStyles;        // +0x144 The states of "ss_stage1.but".
    std::vector<ButtonStyle> mClosedStyles;      // +0x150 The states of "ss_stage2.but".
    std::vector<Rnd::Mesh *> mStageWires;        // +0x15c "ss_butt_wire_0%d.mesh"
    std::vector<Rnd::Mesh *> mIndicatorWires;    // +0x168 "ss_ind_wire_0%d.mesh"
    std::vector<StageListEntry> mStageLevels[5]; // +0x174 The levels of stages 1 through 5.
    std::vector<StageListEntry> mCustomLevels;   // +0x1b0 Emptied and read, but never filled.
    std::vector<StageListEntry> *mCurrentLevels; // +0x1bc The selected stage's list.
    int mSelectedLevel[6];                       // +0x1c0 The level recorded for each stage.
    int mStageLocked[6];                         // +0x1d8 Set for a stage that is not open.
    int mUnlockAll;                              // +0x1f0 MetFrontEndState's `+0x14` at slot 5.
    int mSecretUnlocked;                         // +0x1f4 Set once the first secret level is open.
    int mSuperSecretUnlocked;                    // +0x1f8 Set once the second secret level is open.
    TexturePairRecord mLogoPair;                 // +0x1fc
    TexturePairRecord mLabelPair;                // +0x22c
};
