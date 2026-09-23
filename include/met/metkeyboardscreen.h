#pragma once

#include <vector>

#include "math/vector3.h"
#include "met/metscreen.h"
#include "os/hxstr.h"
#include "rnd/text.h"
#include "rnd/view.h"

class MetKBUser;
struct MetKeyboardRequest;

namespace Rnd {
class Button;
} // namespace Rnd

/**
 * On-screen keyboard.
 *
 * `17MetKeyboardScreen` in the RTTI descriptor at `0x008f29f0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007f5390`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x00282660` takes only the renderer and the load priority, and supplies `kb`
 * for the screen name, `metagame/Shared` for the directory, and `keyboard` for the container. The
 * screen registers under the literal `MetKeyboardScreen` at `0x007f5128`, which slot 30 uses to
 * make itself the active panel again.
 *
 * The object is 0x130 bytes, the size New() requests. The constructor builds an HxStr in place at
 * `+0xb4`, writes 1.0 into the padding word of both offsets at `+0xe0` and `+0xf0`, clears `+0x100`
 * to `+0x118`, writes 1 into `+0x11c` and 5 into `+0x124`, overwrites two members of its own base
 * at `+0x58` and `+0x5c`, and then runs GetDefaultMacros(), discarding the result.
 *
 * Slot 38 resolves seven container objects by name and runs each through the runtime cast helper at
 * `0x005570e0`, with `Rnd::View` and `Rnd::Text` as the two target names the translation unit
 * records at `0x007f5118` and `0x007f5108`. That pairing is what types the seven members.
 * `keypanel_regular.view`, `keypanel_shift.view`, and `keypanel_caps.view` become the three views,
 * and `cursor.txt`, `text entry window.txt`, `title bar.txt`, and `macro_display.txt` become the
 * four texts.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x00283868`, 9 `0x0028c550`, 19 `0x00283268`, 20 `0x0028c518`, 22 `0x0028c4e0`, 23
 * `0x0028c470`, 24 `0x0028c4a8`, 26 `0x0028c708`, 28 `0x0028c5e0`, 30 `0x00283968`, 33
 * `0x0028c808`, 36 `0x00283aa0`, 38 `0x00282948`.
 *
 * The keys are named by the HxStr globals the unit's static initialiser at `0x0028a088` builds
 * from `0x00891b20` onward. Each key's button in the container is `key_<name>.but`, or
 * `key_<name>_cap.but` and `key_<name>_low.but` for a letter. Three layouts of six rows of sixteen
 * names, the regular, shifted, and caps layouts at `0x006a7c98`, `0x006a7cb0`, and `0x006a7cc8`,
 * place the names on the grid mRow and mColumn walk. A key wider than one cell repeats its name in
 * every cell it covers, which is why the movers step until the name changes.
 *
 * The constructor and slot 38 are not written. Slot 38 drives members of Rnd::Text whose
 * signatures are not settled.
 *
 * Every member is private. Nothing outside the class touches one directly.
 */
class MetKeyboardScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * The body is not written, for the reason recorded in the class documentation.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00282660
     */
    MetKeyboardScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00282e30
     */
    virtual ~MetKeyboardScreen();

    /**
     * Produce a keyboard on the heap.
     *
     * The front end's screen factory at `0x00385180` is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The screen.
     * @ghidraAddress 0x0028c358
     */
    static MetScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Record the screen the registered keyboard departs to.
     *
     * No call site exists. The title is inferred.
     *
     * @param returnScreen The registry key of the screen.
     * @ghidraAddress 0x0028c2c0
     */
    static void SetKeyboardReturnScreen(const HxStr &returnScreen);

    /**
     * Report the twelve default keyboard macros, filling the list on first use.
     *
     * The list is a namespace-scope object of the translation unit at `0x006a7c80`. While it does
     * not have exactly twelve entries, it is resized to twelve and each entry is replaced with
     * DefaultMacro() for its index. The constructor and GlobalSettings' constructor both run it.
     *
     * @return The list.
     * @ghidraAddress 0x00284d30
     */
    static std::vector<HxStr> *GetDefaultMacros();

    /**
     * Read one default keyboard macro from the configuration.
     *
     * The key is `kb_macro_f` followed by the one-based index, read under configuration code
     * 0x258.
     *
     * @param nIndex The zero-based macro index.
     * @return The macro text.
     * @ghidraAddress 0x0028cbb8
     */
    static HxStr DefaultMacro(int nIndex);

    /**
     * Fill the registered keyboard from a request and bring it up over the requesting screen.
     *
     * The screen registered as `MetKeyboardScreen` receives the text, the prompt, the receiver, the
     * controller, the ticker text, and the return screen through its setters. The two limits go to
     * the namespace-scope globals at `0x006a7c8c` and `0x006a7c90`, which the key handlers read,
     * and the macro list falls back to the default list when the request has none. The return
     * screen then pushes the keyboard and makes it the active panel. Neither screen lookup is
     * checked for null. MetSaveRemixScreen, MetRemixDelScreen, and MetLoadNewFreqScreen call it.
     * The title is inferred.
     *
     * @param request The request.
     * @ghidraAddress 0x00282468
     */
    static void Open(const MetKeyboardRequest &request);

    /**
     * Disable the twelve function keys.
     *
     * Sets mMacrosDisabled and puts the button of each of the twelve function keys in state 3.
     * ResolveContainerViews() and MetFreqMakerButtonsScreen::HandleCommand() call it. The title is
     * inferred.
     *
     * @ghidraAddress 0x00283c10
     */
    void ResetKeyStates();

    /**
     * Show the keyboard and start its enter animation.
     *
     * Slot 5. Hands the prompt to the title bar and the entered text to the text entry window,
     * puts the caret at the end of the text, selects row 3 column 13, toggles the shift key twice
     * to restore the regular layout and the highlight, starts the caret blink, and runs
     * MetScreen::EnterAndShow().
     *
     * @ghidraAddress 0x00283868
     */
    virtual void EnterAndShow();

    /**
     * Clear the ticker and start the exit animation.
     *
     * Slot 9. Sets the ticker text to the literal `keyboard_clear_ticker` and then runs
     * MetScreen::BeginExit().
     *
     * @ghidraAddress 0x0028c550
     */
    virtual void BeginExit();

    /**
     * Act on one navigation command.
     *
     * Slot 19. Ignores a command from a controller other than mSelector unless mSelector is -1.
     * Every accepted command first clears the pending key. The four directions play the slide
     * sound and move the selection, select presses the selected key, back departs without
     * committing, and codes 7, 8, 11, 12, and 13 press ENTER, SPACE, the left arrow, BACKSPACE,
     * and the right arrow. Codes 20 and 21 act on SHIFT and CAPS at once rather than pressing them.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x00283268
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play the key sound that accompanies sliding.
     *
     * Slot 20. Plays `SND_MET_KEY1` rather than the base's slide sound, and only when the selector
     * matches or the screen accepts every selector.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0028c518
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play the key sound that accompanies the emphasised selection.
     *
     * Slot 22. Plays `SND_MET_KEY2`, restricted the same way as slot 20.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0028c4e0
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * Play the key sound that accompanies cycling a value to the left.
     *
     * Slot 23. Plays `SND_MET_KEY2`, restricted the same way as slot 20.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0028c470
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play the key sound that accompanies cycling a value to the right.
     *
     * Slot 24. Plays `SND_MET_KEY2`, restricted the same way as slot 20.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0028c4a8
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Blink the caret.
     *
     * Slot 26. Returns at once while mBlinkTime is zero, and otherwise waits until the time has
     * passed mBlinkTime plus 240. The caret then toggles, and mBlinkTime advances by 240 from the
     * time rather than from its previous value.
     *
     * @param flTime The current renderer time.
     * @ghidraAddress 0x0028c708
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Apply the pending key and then start the base's alternation.
     *
     * Slot 28. Hides the caret, returns at once when no key is pending, and otherwise dispatches
     * the pending key. The sound mLastAction selects then plays, mLastAction becomes idle, and
     * MetScreen::StartRepeatingSound() runs with the arguments received.
     *
     * @param flStartTime The time the first step runs at.
     * @param flInterval The interval between steps.
     * @param pButton The button whose state alternates.
     * @param nCycles The number of full cycles to run.
     * @ghidraAddress 0x0028c5e0
     */
    virtual void
    StartRepeatingSound(float flStartTime, float flInterval, Rnd::Button *pButton, int nCycles);

    /**
     * Restore the key buttons once a press has finished alternating.
     *
     * Slot 30. Returns at once when no key is pending. Otherwise it restores the pending key's
     * button and the selected key's button, highlights the selected key again, clears the pending
     * key, and makes itself the renderer's active panel again under the literal
     * `MetKeyboardScreen`. The argument the base passes is ignored.
     *
     * @param pButton The button slot 29 finished with, ignored.
     * @ghidraAddress 0x00283968
     */
    virtual void OnUnknownSlot30(Rnd::Button *pButton);

    /**
     * Set the ticker from the recorded text.
     *
     * Slot 33.
     *
     * @ghidraAddress 0x0028c808
     */
    virtual void OnUnknownSlot33();

    /**
     * Report the entered text and depart.
     *
     * Slot 36. Stops the caret blink and restores the selected key's button. When
     * MetScreen::mUnknown18 is set, every trailing space is stripped from the entered text and the
     * text goes to mUser. The screen recorded in mReturnScreen is then told the keyboard was
     * dismissed and made the active panel, and the shift state returns to regular.
     *
     * @ghidraAddress 0x00283aa0
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the three key panels and the four text objects.
     *
     * Slot 38. Runs MetScreen::ResolveContainerViews() first and then resolves each of the seven
     * container objects by name, casting three to Rnd::View and four to Rnd::Text. It records the
     * two caption offsets, selects the regular layout and panel, and runs ResetKeyStates().
     *
     * The body is not written, for the reason recorded in the class documentation.
     *
     * @ghidraAddress 0x00282948
     */
    virtual void ResolveContainerViews();

private:
    // 0x00282ef0
    // Apply one key by name. The named keys go to their handlers, the twelve function keys insert
    // their macro, and every other name is typed as its first character.
    void DispatchKeyName(const HxStr &name);

    // 0x00283f38
    // Highlight the selected key's button and show its macro when it is a function key. Nothing
    // happens on a function key while macros are disabled.
    void HighlightCurrentKey();

    // 0x00284788
    // Step the selection right until the key name changes, wrapping within the row.
    void MoveRight();

    // 0x002848e0
    // Step the selection left until the key name changes, wrapping within the row.
    void MoveLeft();

    // 0x00284a30
    // Step the selection down until the key name changes, wrapping within the column, and past
    // the function keys while macros are disabled.
    void MoveDown();

    // 0x00284bb0
    // Step the selection up, as MoveDown() steps it down.
    void MoveUp();

    // 0x00284ec0
    // Show the macro of a function key beside the caret, or report that it does not fit.
    void ShowMacro(const HxStr &key);

    // 0x002850b8
    // Append the default macro of one function key to the text when it fits.
    void InsertMacro(int nIndex);

    // 0x00285378. Inline, with this out-of-line copy.
    // Find the button of a key. A single letter uses its case-specific button.
    Rnd::Button *FindKeyButton(const HxStr &key);

    // 0x00285b08
    // Toggle shift, which caps also releases.
    void OnShift();

    // 0x00285e28
    // Delete the character before the caret.
    void OnBackspace();

    // 0x00285fa0
    // Move the caret left.
    void OnCaretLeft();

    // 0x00286120
    // Move the caret right.
    void OnCaretRight();

    // 0x002862b0
    // Toggle caps lock, which also releases shift.
    void OnCaps();

    // 0x002865a0
    // Insert three spaces when they fit.
    void OnTab();

    // 0x00286850
    // Insert one space when it fits.
    void OnSpace();

    // 0x00286ab0
    // Delete the character at the caret.
    void OnDelete();

    // 0x00286c10
    // Insert the first character of a key name when it fits.
    void OnCharacter(const HxStr &key);

    // 0x0028c3e8. Inline, with this out-of-line copy.
    // Show the caret beside the character it precedes.
    void UpdateCursor();

    // 0x0028c7c0
    // Hide the caret and restart its blink.
    void ResetCaret();

    // 0x0028c828
    // Empty in the image. Slot 36 calls it last. The title is inferred.
    void OnDeparted();

    // 0x0028c870. Inline, with this out-of-line copy.
    // The name of the selected key.
    HxStr *CurrentKey();

    // 0x0028c898. Inline, with this out-of-line copy.
    // Put a key's button back in its resting state, latched for an active shift or caps key.
    void UnhighlightKey(const HxStr &key);

    // 0x0028c9c8. Inline, with this out-of-line copy and no call site.
    // Remove one character of the text.
    void RemoveChar(int nIndex);

    // 0x0028c9e8. Inline, with this out-of-line copy and no call site.
    // Append to the text.
    void AppendText(const HxStr &text);

    // 0x0028ca08. Inline, with this out-of-line copy and no call site.
    // Insert into the text at a position, or append past its end.
    void InsertText(const HxStr &text, unsigned nPos);

    // 0x0028ca58. Inline, with this out-of-line copy.
    // The horizontal position the end of the text is laid out at, truncated.
    int TextEndX();

    // 0x0028caf8. Inline, with this out-of-line copy.
    // Clear the macro caption.
    void HideMacro();

    // 0x0028cc50
    // Whether a key is one of the twelve function keys.
    bool IsMacroKey(const HxStr &key);

    // 0x0028ccb8
    // Record the key a press is applying.
    void SetPendingCommand(const HxStr &key);

    // 0x0028cd00. Inline, with this out-of-line copy.
    // Replace the shared ticker text with the argument and repost it at the renderer's current
    // time, doing nothing when the text has not changed. The text is a function-local static
    // HxStr at 0x00891b18 behind the guard flag at 0x006a7ce0, and 0x0028ccd8 is its destructor.
    void SetTickerText(const HxStr &text);

    // 0x0028cda8
    // Commit the text and depart.
    void OnEnter();

    // Press a key as the controller's shortcut commands do. Inline at each of HandleCommand()'s
    // five sites, with no out-of-line copy.
    void PressKey(const HxStr &key);

    // 0x0028ce70
    // Insert the macro of one function key and move the caret past it.
    void OnMacro(int nIndex);

    // 0x0028c830. Assigns the entered text. Open() is the one caller.
    void SetText(const HxStr &text);

    // 0x0028c850. Assigns the prompt. Open() is the one caller.
    void SetPrompt(const HxStr &prompt);

    // 0x0028cad0. Records the receiver of the committed text. Open() is the one caller.
    void SetUser(MetKBUser *pUser);

    // 0x0028c3e0. Records the one controller the keyboard accepts. Open() is the one caller.
    void SetSelector(int nSelector);

    // 0x0028cad8. Assigns the ticker text slot 33 posts. Open() is the one caller.
    void SetTicker(const HxStr &ticker);

    // 0x0028cab0. Assigns the registry key of the screen slot 36 departs to.
    void SetReturnScreen(const HxStr &returnScreen);

    // The macro list the keyboard offers. Open() stores the request's list here, or the default
    // list when the request has none. +0x8c
    std::vector<HxStr> *mMacros;
    Rnd::View *mpKeypanelRegular; // +0x90, `keypanel_regular.view`
    Rnd::View *mpKeypanelShift;   // +0x94, `keypanel_shift.view`
    Rnd::View *mpKeypanelCaps;    // +0x98, `keypanel_caps.view`
    Rnd::Text *mpCursor;          // +0x9c, `cursor.txt`
    Rnd::Text *mpTextEntryWindow; // +0xa0, `text entry window.txt`
    Rnd::Text *mpTitleBar;        // +0xa4, `title bar.txt`
    Rnd::Text *mpMacroDisplay;    // +0xa8, `macro_display.txt`
    // Registry key of the screen slot 36 departs to. +0xac
    HxStr mReturnScreen;
    // The entered text. +0xb4
    HxStr mText;
    // The prompt SetPrompt() assigns. +0xbc
    HxStr mPrompt;
    // The ticker text slot 33 posts. +0xc4
    HxStr mTicker;
    // The key a press is applying, empty while no press is pending. +0xcc
    HxStr mPendingKey;
    // The number of characters before the caret. +0xd4
    int mCaret;
    // Not written by the constructor.
    unsigned char mUnknownd8[0x08]; // +0xd8
    // Offset of the caret from the character it precedes. +0xe0
    Vector3 mCursorOffset;
    // Offset of the macro caption from the character it follows. +0xf0
    Vector3 mMacroOffset;
    // When the caret next toggles, or zero while it does not blink. +0x100
    float mBlinkTime;
    // The receiver of the committed text, which SetUser() records. +0x104
    MetKBUser *mUser;
    // The rows of the active layout. +0x108
    HxStr **mRows;
    // The visible key panel. +0x10c
    Rnd::View *mPanel;
    int mRow;            // +0x110
    int mColumn;         // +0x114
    int mShiftState;     // +0x118, a KeyboardShiftState
    int mMacrosDisabled; // +0x11c, starts at 1
    // The selector the four sound slots and slot 19 compare their argument against. A screen that
    // records -1 accepts every value. SetSelector() is the one writer, and the constructor does not
    // clear it.
    int mSelector; // +0x120
    // What the last key did, a KeyboardAction that slot 28 turns into a sound. Starts at 5.
    int mLastAction; // +0x124
};
