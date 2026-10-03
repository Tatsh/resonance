#pragma once

#include "met/metremixsaver.h"
#include "met/metscreen.h"
#include "met/texturepairrecord.h"

namespace Rnd {
class Mat;
class Tex;
class Text;
} // namespace Rnd

/**
 * End-of-remix screen for a solo session.
 *
 * Its RTTI descriptor is at `0x008f07f0`. It has two public non-virtual bases at fixed offsets,
 * MetScreen at `+0x00` and MetRemixSaver at `+140`. The factory at `0x00399630` confirms the object
 * size of 0x11c bytes by requesting exactly that many with the tag `MsgSink`. The 39-entry primary
 * vtable is at `0x0080c4f0`, the same length as the MetScreen table, and the class declares no new
 * virtual. The five-entry MetRemixSaver table at `0x0080c4c0` adjusts `this` by `-140` in every
 * entry. That table is where this screen supplies the three MetRemixSaver pure
 * virtuals.
 *
 * The constructor at `0x003943f8` takes only the renderer and the load priority, and supplies
 * `erss` for the screen name, `metagame/_Solo` for the directory, and `end_remix` for the
 * container. It builds the two TexturePairRecord members from the texture pairs `gSongLogo1.tex`
 * with `gSongLogo2.tex` and `gSongLabel1.tex` with `gSongLabel2.tex`.
 *
 * The destructor at `0x003996b8` restores both vptrs, runs the TexturePairRecord destructor at
 * `0x001fc568` on each of the two records, restores the MetRemixSaver vptr to `0x007ffcd0`, runs
 * the MetScreen destructor, and releases the object with the tag `MsgSink`. Every step is
 * compiler-generated member destruction or a vptr restore, so the definition is empty.
 *
 * Seven entries of the primary table differ from the MetScreen table, which a diff of the two
 * tables settles. They are 0 `0x003995b0`, the
 * compiler-generated GetTypeInfo, 1 `0x003996b8` the destructor, 5 `0x00394e10`, 7 `0x003997d0`,
 * 26 `0x00399748`, 36 `0x00399870`, and 38 `0x00394728`. All five behaviour slots are declared
 * below.
 *
 * The three MetRemixSaver pure virtuals are supplied at `0x003998c8`, `0x00399898`, and
 * `0x00395918`, and all three are declared below with the spelling the base gives them.
 *
 * Slots 2, 3, 4, 7, and 36 all route through one private helper or one of the two navigation
 * virtuals, and mScreenExited is the flag that selects between them. Slot 3 sets it, slot 4 stores
 * the negation of its argument in it, and slots 2 and 36 branch on it.
 */
class MetSoloEndRemixScreen : public MetScreen, public MetRemixSaver {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x003943f8
     * @ghidraAddress PAL: 0x003c5eb8
     */
    MetSoloEndRemixScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x003996b8
     * @ghidraAddress PAL: 0x003cb810
     */
    virtual ~MetSoloEndRemixScreen();

    /**
     * Allocate and construct the screen.
     *
     * The 0x11c-byte allocation is billed to the tag `MsgSink`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x00399630
     * @ghidraAddress PAL: 0x003cb788
     */
    static MetSoloEndRemixScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Show the screen, or first save the global settings when the front end asks for it. Slot 5.
     *
     * When MetFrontEndState's `+0x0c` and `+0x10` flags are both 1, `+0x10` is cleared,
     * MetGlobalSettingsSaverScreen::StartSave() runs with this screen as the one to return to, and
     * mActivatePending is cleared. Otherwise ShowResults() fills and shows the screen.
     *
     * @ghidraAddress NTSC-U/C: 0x00394e10
     * @ghidraAddress PAL: 0x003c6ad8
     */
    virtual void EnterAndShow();

    /**
     * Make the save screen the active panel. Slot 7.
     *
     * Activates the panel registered under `MetSaveRemixScreen` and does nothing else. MetScreen
     * slot 6 is its one caller.
     *
     * @ghidraAddress NTSC-U/C: 0x003997d0
     * @ghidraAddress PAL: 0x003cb928
     */
    virtual void OnPanelActivated();

    /**
     * Advance both texture pairs and show each one's current texture. Slot 26.
     *
     * The song logo pair feeds the first stage of mLogoMat and the song label pair the first stage
     * of mPhotoMat. The float argument the slot receives is not read.
     *
     * @param flTime The current renderer time, which the body does not read.
     * @ghidraAddress NTSC-U/C: 0x00399748
     * @ghidraAddress PAL: 0x003cb8a0
     */
    virtual void UpdateIdle(float flTime);

    /**
     * Run the departure sequence once, unless a save is still pending. Slot 36.
     *
     * MetScreen slot 35 runs this slot once the exit animation has finished. A set mScreenExited
     * records that slot 3 or slot 4 has already requested the departure, and the sequence then does
     * not run a second time.
     *
     * @ghidraAddress NTSC-U/C: 0x00399870
     * @ghidraAddress PAL: 0x003cb9e8
     */
    virtual void OnExitFinished();

    /**
     * Resolve the container objects this screen drives. Slot 38.
     *
     * Runs the MetScreen slot 38 body first. It then labels the remix panel text from configuration
     * code 0x258 without storing it, and resolves the six text fields, the three materials, and the
     * first persona burn texture into the members at `+0x90` through `+0xb4`. Only the panel text
     * is used without a null test.
     *
     * @ghidraAddress NTSC-U/C: 0x00394728
     * @ghidraAddress PAL: 0x003c62b8
     */
    virtual void ResolveContainerViews();

    /**
     * Depart the screen, or run the departure sequence when one is already pending.
     *
     * MetRemixSaver slot 2. A clear mScreenExited starts the exit animation and lets slot 36 run
     * the sequence once the animation has finished. A set mScreenExited runs the sequence at once.
     *
     * @param bCompleted Not read.
     * @ghidraAddress NTSC-U/C: 0x003998c8
     * @ghidraAddress PAL: 0x003cba40
     */
    virtual void OnSaveFinished(int bCompleted);

    /**
     * Record that the departure sequence has been requested and start the exit animation.
     *
     * MetRemixSaver slot 3.
     *
     * @ghidraAddress NTSC-U/C: 0x00399898
     * @ghidraAddress PAL: 0x003cba10
     */
    virtual void OnHelpRequested();

    /**
     * Show or exit this screen and record which of the two happened.
     *
     * MetRemixSaver slot 4. The member takes the negation of the argument, which is faithful only
     * for an argument of 0 or 1, because the binary computes it with `xori` against 1 rather than
     * with a comparison.
     *
     * @param bShowing Non-zero to bring the screen back onto the stack, zero to exit it.
     * @ghidraAddress NTSC-U/C: 0x00395918
     * @ghidraAddress PAL: 0x003c78a8
     */
    virtual void SetOwnerScreenShowing(int bShowing);

private:
    // NTSC-U/C: 0x00395a20, PAL: 0x003c79e0
    // Resolves the arena view, runs the renderer's two empty hooks, pushes `MetHelpScreen`,
    // `MetScreenTitleScreen`, and `MetRemixTypeScreen`, and activates `MetRemixTypeScreen` as the
    // panel. Slots 2 and 36 are its two callers.
    void ReturnToTitle();

    // NTSC-U/C: 0x00395058, PAL: 0x003c6d68
    // Loads both texture pairs for the session's level and fills the song, date, and username
    // fields from configuration codes 0x320 through 0x327, falling back to the short title when the
    // full one exceeds the title's wrap width. It then shows the persona's burn texture on the face
    // material's second stage, sets the title-screen caption, pushes `MetHelpScreen`, opens the
    // save screen for the first persona's appearance on the first card slot, and runs
    // MetScreen::EnterAndShow(). EnterAndShow() is its one caller, and the title is inferred. The
    // European release saves to a copy of the first card slot only when that slot is in port 1,
    // and otherwise to the stand-in slot `1`.
    void ShowResults();

    // ResolveContainerViews() fills +0x90 through +0xb4, and the constructor writes none of them.
    Rnd::Text *mBpmText;      // +0x90 "ers_bpm.txt"
    Rnd::Text *mGenreText;    // +0x94 "ers_genre.txt"
    Rnd::Text *mTitleText;    // +0x98 "ers_title.txt"
    Rnd::Text *mArtistText;   // +0x9c "ers_artist.txt"
    Rnd::Text *mDateText;     // +0xa0 "ers_date.txt"
    Rnd::Text *mFreqNameText; // +0xa4 "ers_freqname_remix.txt"
    Rnd::Mat *mFaceMat;       // +0xa8 "ers_face.mat"
    Rnd::Tex *mBurnTex;       // +0xac The first persona burn texture.
    Rnd::Mat *mLogoMat;       // +0xb0 "ers_logo.mat"
    Rnd::Mat *mPhotoMat;      // +0xb4 "ers_photo.mat"
    // Set once this screen has exited the stack for the help screen or a declined save. Slot 3
    // sets it, slot 4 stores the negation of its argument in it, and slots 2 and 36 branch on it.
    // The constructor never writes it. A screen whose slot 36 runs before either setter therefore
    // reads an indeterminate value.
    int mScreenExited; // +0xb8
    // The two texture pairs the screen flips between, built from the song logo and the song label
    // pairs in that order.
    TexturePairRecord mLogoTextures;  // +0xbc
    TexturePairRecord mLabelTextures; // +0xec
};
