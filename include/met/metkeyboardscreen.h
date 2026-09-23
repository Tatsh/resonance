#pragma once

#include "met/metscreen.h"
#include "os/hxstr.h"
#include "rnd/text.h"
#include "rnd/view.h"

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
 * The object is at least 0x128 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the highest store the constructor makes plus its
 * width. An earlier reading recorded 0xd8 and described the constructor as writing only two runs,
 * `+0x8c` to `+0xb0` and `+0xbc` to `+0xd4`. The constructor also builds an HxStr in place at
 * `+0xb4`, writes 1.0 into `+0xec` and `+0xfc`, clears `+0x100` to `+0x118`, writes 1 into `+0x11c`
 * and 5 into `+0x124`, overwrites two members of its own base at `+0x58` and `+0x5c`, and then runs
 * the further initialiser at `0x00284d30`.
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
 * Slot 19 is the input-command handler and it is the routine that settles what the sound slots
 * take. It receives one pointer to a 16-byte controller-command record, reads a selector from
 * `+0x00` and a second value from `+0x04`, returns at once unless mSelector is -1 or equals that
 * second value, and then switches the selector over 21 cases through the table at `0x007f5230`. The
 * record has no descriptor, no embedded file path, and no method name anywhere in the image. It is
 * embedded in a ControllerCmd at `+0x0c`, which ControllerCmd::Execute at `0x00194560` proves by
 * passing `this + 0x0c` on to `0x0018f078`. It is not titled here, and slot 19 is therefore
 * documented rather than declared. An earlier reading described the field at `+0x04` as a sequence
 * number; this class compares it against its own selector, so it is the same kind of value as
 * mSelector.
 *
 * Six slots are documented rather than written. Slot 5 and slot 38 both drive members of Rnd::Text
 * whose signatures are not settled, and slots 26, 28, 30, and 36 run the class's own key-name
 * dispatcher at `0x00282ef0`, which resolves 24 HxStr globals from `0x00891b30` onward to their
 * handlers and has no recovered titles for them.
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
     * Show the keyboard and start its enter animation.
     *
     * Slot 5. Hands the two panel captions to the title bar and the text entry window, copies the
     * entered text's length into mUnknownd4, clears the text entry window, measures the caret
     * position through `0x004c9e98`, moves the cursor there, records 3 in mUnknown110 and 13 in
     * mUnknown114, resets the key-repeat clock to 1.0, and runs MetScreen::EnterAndShow().
     *
     * The body is not written, for the reason recorded in the class documentation.
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
     * Repeat the current key once the repeat interval has elapsed.
     *
     * Slot 26. Returns at once while the repeat clock is zero, and otherwise waits until the time
     * has passed the clock plus 240. The clock then advances by 240 from the time rather than from
     * its previous value, so the interval is measured from the call that fired it. The cursor's
     * slot 1 runs with 0 when the cursor reports a set field at `+0x04` and with 1 otherwise, which
     * is what makes the caret blink.
     *
     * The body is not written, for the reason recorded in the class documentation.
     *
     * @param flTime The current renderer time.
     * @ghidraAddress 0x0028c708
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Apply the pending key command and then start the base's alternation.
     *
     * Slot 28. Resets the caret through the helper at `0x0028c7c0`, returns at once when the
     * pending command name is empty, and otherwise dispatches the name through the class's key-name
     * dispatcher. It then switches mUnknown124 over seven cases through the table at `0x007f5980`,
     * runs one of slots 25, 20, 22, 23, and 24 with its own selector, records 6, and finally runs
     * MetScreen::StartRepeatingSound() with the arguments it received.
     *
     * The body is not written, for the reason recorded in the class documentation.
     *
     * @param flStartTime The time the first step runs at.
     * @param flInterval The interval between steps.
     * @param pObject The object whose material state alternates.
     * @param nCycles The number of full cycles to run.
     * @ghidraAddress 0x0028c5e0
     */
    virtual void
    StartRepeatingSound(float flStartTime, float flInterval, Rnd::Object *pObject, int nCycles);

    /**
     * Commit the pending key command.
     *
     * Slot 30. Returns at once when the pending command name is empty. Otherwise it applies the
     * name, applies a second name the helper at `0x0028c870` resolves, runs the class's own
     * `0x00283f38`, clears the pending name, and makes itself the renderer's active panel again
     * under the literal `MetKeyboardScreen`. The argument the base passes is ignored.
     *
     * The body is not written, for the reason recorded in the class documentation.
     *
     * @param pObject The object slot 29 finished with, ignored.
     * @ghidraAddress 0x00283968
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Report the entered text and depart.
     *
     * Slot 36. Clears the repeat clock, applies the name the helper at `0x0028c870` resolves, and
     * then, when MetScreen::mUnknown18 is set, strips every trailing space from the entered text
     * before handing it to the object at `+0x104`. It resolves the screen recorded in mUnknownac,
     * runs slot 11 on it, activates it, runs the class's `0x0028c828`, and clears mUnknown118.
     *
     * The body is not written, for the reason recorded in the class documentation.
     *
     * @ghidraAddress 0x00283aa0
     */
    virtual void OnUnknownSlot36();

    /**
     * Set the ticker from the recorded text.
     *
     * Slot 33.
     *
     * @ghidraAddress 0x0028c808
     */
    virtual void OnUnknownSlot33();

    /**
     * Resolve the three key panels and the four text objects.
     *
     * Slot 38. Runs MetScreen::ResolveContainerViews() first and then resolves each of the seven
     * container objects by name, casting three to Rnd::View and four to Rnd::Text. It ends by
     * recording four values in mUnknown108 through mUnknown114 and running `0x00283c10`.
     *
     * The body is not written, for the reason recorded in the class documentation.
     *
     * @ghidraAddress 0x00282948
     */
    virtual void ResolveContainerViews();

private:
    // 0x0028cd00
    // Replaces the shared ticker text with the argument and reposts it at the
    // renderer's current time, doing nothing when the text has not changed. The text itself is a
    // function-local static HxStr at 0x00891b18 behind the guard flag at 0x006a7ce0, initialised to
    // the empty string. The body is not written. The repost runs through 0x00317368 and that
    // routine has no recovered title.
    void SetTickerText(const HxStr &text);

    int mUnknown8c;               // +0x8c
    Rnd::View *mpKeypanelRegular; // +0x90, `keypanel_regular.view`
    Rnd::View *mpKeypanelShift;   // +0x94, `keypanel_shift.view`
    Rnd::View *mpKeypanelCaps;    // +0x98, `keypanel_caps.view`
    Rnd::Text *mpCursor;          // +0x9c, `cursor.txt`
    Rnd::Text *mpTextEntryWindow; // +0xa0, `text entry window.txt`
    Rnd::Text *mpTitleBar;        // +0xa4, `title bar.txt`
    Rnd::Text *mpMacroDisplay;    // +0xa8, `macro_display.txt`
    // Registry key of the screen slot 36 departs to.
    HxStr mUnknownac; // +0xac
    // The entered text. Slot 36 strips its trailing spaces.
    HxStr mUnknownb4; // +0xb4
    int mUnknownbc;   // +0xbc
    int mUnknownc0;   // +0xc0
    // The ticker text slot 33 posts.
    HxStr mUnknownc4; // +0xc4
    // The pending key command name. Slots 28, 30, and 36 all test it for emptiness first.
    HxStr mUnknowncc; // +0xcc
    int mUnknownd4;   // +0xd4
    // Not written by the constructor.
    unsigned char mUnknownd8[0x14]; // +0xd8
    float mUnknownec;               // +0xec, starts at 1.0f
    // Not written by the constructor.
    unsigned char mUnknownf0[0xc]; // +0xf0
    float mUnknownfc;              // +0xfc, starts at 1.0f
    // Deadline of the key repeat, or zero while no key repeats. Slot 26 advances it by 240 a step.
    float mUnknown100; // +0x100
    int mUnknown104;   // +0x104
    int mUnknown108;   // +0x108
    int mUnknown10c;   // +0x10c
    int mUnknown110;   // +0x110
    int mUnknown114;   // +0x114
    int mUnknown118;   // +0x118
    int mUnknown11c;   // +0x11c, starts at 1
    // The selector the four sound slots and slot 19 compare their argument against. A screen that
    // records -1 accepts every value. Nothing in this class writes it, and neither the constructor
    // nor the initialiser at 0x00284d30 clears it, so the writer is not recovered.
    int mSelector; // +0x120
    // Index of the last key action, switched over seven cases by slot 28. Starts at 5.
    int mUnknown124; // +0x124
};
