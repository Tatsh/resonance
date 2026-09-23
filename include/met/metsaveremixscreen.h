#pragma once

#include <vector>

#include "met/metsaveremix.h"
#include "os/hxstr.h"

class MetButtonList;
class MetRemixSaver;

namespace Rnd {
class Object;
class Text;
} // namespace Rnd

/**
 * Solo screen that writes a finished remix to a memory card.
 *
 * `18MetSaveRemixScreen` in the RTTI descriptor at `0x008ef8c0`, with MetSaveRemix as its one
 * public non-virtual base at offset 0. Its own members start at `+0xe8`, which fixes the size of
 * MetSaveRemix, and the object is 0x10c bytes, which the factory at `0x003817e0` pins by requesting
 * exactly that many with the tag `MsgSink`. That factory is one inlined
 * `new MetSaveRemixScreen(renderer, priority)` expression emitted out of line, and the tag is
 * MsgSink's rather than this class's, because MsgSink is the base that declares `operator new`.
 * Three vtables belong to the class, the
 * 43-entry primary at `0x0080a690`, the 21-entry MemcardUser table at `0x0080a5e0` that adjusts
 * `this` by `-140`, and the three-entry MetKBUser table at `0x0080a5c0` that adjusts it by `-144`.
 * The primary is the same length as the MetSaveRemix table, so the class declares no virtual of
 * its own. The MemcardUser table overrides no slot of its own and inherits the four MetSaveRemix
 * supplies.
 *
 * The constructor at `0x0037ace0` takes only the renderer and the load priority. It runs the
 * MetSaveRemix constructor at `0x00372120` with `ers` for the screen name, `metagame/_Solo` for
 * the directory, and `save_remix` for the container, writes its own three vptrs, zeroes
 * mUnknowne8, mUnknownec, and mUnknown100, default-constructs mUnknown104, allocates a
 * MetButtonList tagged `MetButtonList` into mUnknowne8, and pushes `remix_save` into the
 * container object-name vector MetScreen declares at `+0x38`. It then clears
 * MetScreen::mUnknown5c, which is why MetScreen::mUnknown5c is protected rather than private, and
 * MetSaveRemix::mUnknowne0, which requires that MetSaveRemix member to be protected as well. The
 * image emits the mUnknown5c store on both paths of the temporary release above it, which is one
 * source statement rather than two.
 *
 * The destructor at `0x00381868` restores the three vptrs, deletes mUnknowne8 through slot 1 of
 * the MetButtonList table with the deleting `__in_chrg` value, releases the mUnknown104 buffer
 * through the inlined HxStr destructor, runs the MetSaveRemix destructor, and releases the object
 * with the tag `MsgSink`.
 *
 * Seventeen entries of the primary table differ from the MetSaveRemix table, which a diff of the
 * two tables settles rather than the title each routine carries. They are 0 `0x00381768`, the
 * compiler-generated GetTypeInfo, 1 `0x00381868` the destructor, 5 `0x0037b8e8`, 7 `0x0037b718`,
 * 15 `0x0037cac8`, 19 `0x0037b258`, 20 `0x00381968`, 21 `0x003817d8`, 22 `0x003817c0`,
 * 23 `0x003817c8`, 24 `0x003817d0`, 30 `0x0037c110`, 36 `0x0037c260`, 38 `0x0037af98`,
 * 40 `0x003819f0`, 41 `0x00381a28`, and 42 `0x0037ccc8`. All fifteen behaviour slots are declared
 * below. Slots 21 through 24 sit eight bytes apart and are two-instruction `jr ra` stubs, and the
 * declaration order there puts leave before high, so their addresses do not ascend with the slot
 * numbers.
 *
 * The three-entry MetKBUser table overrides its one slot at `0x00381990`, which is declared below
 * with the spelling the base gives it.
 *
 * Two of the words in the span the constructor never writes are objects. mUnknownf8 is the
 * Rnd::Text the entered name is drawn into, which slot 38 resolves by name and narrows with a
 * dynamic_cast to Rnd::Text. The word at `+0xfc` is the MetRemixSaver subobject of the screen
 * that asked for the save, which Open() stores through SetSaver(). Both callers of Open() pass
 * their own subobject at `+140`, where MetMultiSaveRemixScreen and MetSoloEndRemixScreen place
 * MetRemixSaver.
 */
class MetSaveRemixScreen : public MetSaveRemix {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0037ace0
     */
    MetSaveRemixScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00381868
     */
    virtual ~MetSaveRemixScreen();

    /**
     * Fill the registered save screen with a save request and bring it up over MetLoadGameScreen.
     *
     * The screen registered as `MetSaveRemixScreen` receives each argument through the setters
     * below and the location through a direct assignment to MetSaveRemix::mUnknown94. A non-zero
     * bClearName assigns the empty string to `+0x104`. MetLoadGameScreen then pushes the save
     * screen and makes it the active panel. Neither screen lookup is checked for null.
     * MetMultiSaveRemixScreen's slots 2 and 36 and MetSoloEndRemixScreen's routine at `0x00395058`
     * call it. The title is inferred.
     *
     * @param nUnknownec Stored through SetUnknownec().
     * @param nPad The controller that owns the save, stored through SetOwnerPad().
     * @param pSaver The screen to report back to, stored through SetSaver().
     * @param slot The memory-card location to save to.
     * @param appearances The players' appearances, stored through SetAppearances().
     * @param bClearName Non-zero to empty `+0x104` through SetUnknown104().
     * @ghidraAddress 0x0037a9e0
     */
    static void Open(int nUnknownec,
                     int nPad,
                     MetRemixSaver *pSaver,
                     const MemcardConnectState &slot,
                     const std::vector<FreqAppearance> &appearances,
                     int bClearName);

    /**
     * Store the word at `+0xec`.
     *
     * @param nUnknownec The value.
     * @ghidraAddress 0x00381910
     */
    void SetUnknownec(int nUnknownec);

    /**
     * Store the controller that owns the save in MetSaveRemix::mUnknownc8.
     *
     * @param nPad The controller index.
     * @ghidraAddress 0x00381918
     */
    void SetOwnerPad(int nPad);

    /**
     * Store the screen slots 40 and 41 report back to.
     *
     * @param pSaver The screen.
     * @ghidraAddress 0x00381920
     */
    void SetSaver(MetRemixSaver *pSaver);

    /**
     * Copy the players' appearances into MetSaveRemix::mUnknownac.
     *
     * @param appearances The appearances.
     * @ghidraAddress 0x00381928
     */
    void SetAppearances(const std::vector<FreqAppearance> &appearances);

    /**
     * Assign the string at `+0x104`.
     *
     * @param text The new text.
     * @ghidraAddress 0x00381948
     */
    void SetUnknown104(const HxStr &text);

    /**
     * Show the screen and fill it from the finished session. Slot 5.
     *
     * The body is not written. It makes itself the renderer's active panel through
     * MetRenderer::SetActivePanel(), runs the empty MetRenderer routine at `0x00390088`, resets the
     * MetButtonList at mUnknowne8, and then formats several fields with FormatString() from
     * data-array property lookups at `0x005096d0`, resolving `ers_player_pan.txt` and further
     * objects by name and narrowing each with a dynamic_cast to Rnd::Text. It stores the result of
     * the game-manager query at `0x002156b0` in mUnknownec, and ends by copying one MetRemixRecord
     * out of the MetRemixManager. Neither the property lookup nor the manager routines are
     * identified.
     *
     * @ghidraAddress 0x0037b8e8
     */
    virtual void EnterAndShow();

    /**
     * Run the pending keyboard action once the panel has finished activating. Slot 7.
     *
     * The body is not written. A clear MetSaveRemix::mUnknowne0 takes a second path that reads the
     * renderer instead, and a null word at `+0xfc` returns at once. Otherwise it clears
     * mUnknowne0, exits `MetHelpScreen`, and runs for roughly 0x180 further instructions.
     *
     * @ghidraAddress 0x0037b718
     */
    virtual void OnUnknownSlot7();

    /**
     * Act on the discard confirmation. Slot 15.
     *
     * The body is not written. It compares the dialogue name against `discard_remix` through
     * HxStr::MatchesLiteral and returns at once when it differs, then branches on the choice
     * against 1. The accepting branch clears MetScreen::mUnknown18 and mUnknown100 and dispatches
     * slot 36.
     *
     * @param name The dialogue the screen requested, which the message screen reports back.
     * @param nChoice Which of the dialogue's buttons the user chose, counted from zero.
     * @ghidraAddress 0x0037cac8
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Act on a navigation command from the one controller that owns the screen. Slot 19.
     *
     * The body is not written. A command whose pad index differs from MetSaveRemix::mUnknownc8 is
     * discarded before anything else, which is the same test the sound override at slot 20 makes
     * and is what fixes mUnknownc8 as the owning controller index. Of the codes that pass, 5 reads
     * the Rnd::Text at mUnknownf8 and 7 runs from `0x0037b398`, and every other code returns.
     *
     * @param pCommand The command the renderer translated from an input message.
     * @ghidraAddress 0x0037b258
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the slide sound when the selector matches MetSaveRemix::mUnknownc8.
     *
     * This is the one sound override of the class that is not an empty stub, and the only one in
     * the band that compares the selector against a recorded value.
     *
     * @param nSelector Compared against MetSaveRemix::mUnknownc8, then passed through unchanged.
     * @ghidraAddress 0x00381968
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Silence the leave sound.
     *
     * The four overrides below are two-instruction stubs, so each was written inline with an empty
     * body. Their declaration order puts leave before high, which is why their addresses do not
     * ascend with the slot numbers.
     *
     * @ghidraAddress 0x003817d8
     */
    virtual void PlayLeaveSound() {
    }

    /**
     * Silence the high sound.
     *
     * @ghidraAddress 0x003817c0
     */
    virtual void PlayHighSound(int) {
    }

    /**
     * Silence the cycle-left sound.
     *
     * @ghidraAddress 0x003817c8
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * @ghidraAddress 0x003817d0
     */
    virtual void PlayCycleRightSound(int) {
    }

    /**
     * Depart to the help screen. Slot 30.
     *
     * The body is not written. It records 2 in MetScreen::mUnknown18, dispatches slot 3 of the
     * object at `+0xfc` when that word is set, and exits `MetHelpScreen`. MetScreen slot 29 passes
     * the object it finished alternating, and this body does not read it.
     *
     * @param pObject The object slot 29 finished with, which the body does not read.
     * @ghidraAddress 0x0037c110
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Act on the button the user chose once the exit animation has finished. Slot 36.
     *
     * The body is not written. It returns at once unless mUnknown100 is set, clears it, and then
     * resolves the game manager and branches for roughly 0x340 instructions on
     * MetScreen::mUnknown18 and on the entry the button list reports.
     *
     * @ghidraAddress 0x0037c260
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the three text objects the screen draws into. Slot 38.
     *
     * The body is not written. It runs the MetSaveRemix slot 38 body, which is MetScreen's, then
     * resolves three objects by name through Rnd::Manager::Find(), narrowing each with a
     * dynamic_cast to Rnd::Text, and stores the last of the three in mUnknownf8. It ends by adding
     * one entry to the MetButtonList at mUnknowne8 through `0x001fcb28`, which is not declared.
     *
     * @ghidraAddress 0x0037af98
     */
    virtual void ResolveContainerViews();

    /**
     * Dispatch slot 2 of the object at `+0xfc` with a zero argument. Slot 40.
     *
     * A null word at `+0xfc` does nothing. The body is not written, because MetRemixSaver declares
     * slot 2 without the integer argument this dispatch passes in a1.
     *
     * @ghidraAddress 0x003819f0
     */
    virtual void OnUnknownSlot40();

    /**
     * Dispatch slot 2 of the object at `+0xfc` with an argument of one. Slot 41.
     *
     * The body differs from slot 40's in that one immediate and nothing else, and is not written
     * for the same reason.
     *
     * @ghidraAddress 0x00381a28
     */
    virtual void OnUnknownSlot41();

    /**
     * Request a remix name from the on-screen keyboard. Slot 42.
     *
     * The body is not written. It sets MetSaveRemix::mUnknowne0 and then runs the six-argument
     * keyboard entry point at `0x0028cf18` with this screen's own registry key as the screen to
     * return to, `Remix name` as the prompt, and this object's own MetKBUser subobject as the
     * receiver, exactly as the MetRemixDelScreen override does. That entry point belongs to
     * MetKeyboardScreen and is neither titled nor declared.
     *
     * @ghidraAddress 0x0037ccc8
     */
    virtual void OnUnknownSlot42();

    /**
     * Draw the entered name and forward it to the saver once. MetKBUser slot 2.
     *
     * The text reaches the Rnd::Text at mUnknownf8 whether or not a save is pending. A clear
     * MetSaveRemix::mUnknowne0 then discards it, which is how a keyboard result this screen did not
     * request is ignored.
     *
     * @param text The remix name the user entered.
     * @ghidraAddress 0x00381990
     */
    virtual void OnUnknownSlot2(const HxStr &text);

private:
    MetButtonList *mUnknowne8; // +0xe8
    // Slot 5 stores the result of the game-manager query at `0x002156b0` here. +0xec
    int mUnknownec;
    // +0xf0 through +0xf7 are not written by the constructor and are not recovered.
    unsigned char mUnknownf0[0x8]; // +0xf0
    // The text object the entered name is drawn into. Slot 38 resolves it. +0xf8
    Rnd::Text *mUnknownf8;
    // The screen that asked for the save, which Open() stores through SetSaver(). Slots 30, 40,
    // and 41 dispatch its slots 2 and 3. +0xfc
    MetRemixSaver *mUnknownfc;
    int mUnknown100; // +0x100
    // Default-constructed, and its buffer is what the destructor releases at `+0x108`. +0x104
    HxStr mUnknown104;
};
