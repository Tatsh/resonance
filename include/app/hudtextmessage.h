#pragma once

#include "rnd/font.h"

class HxStr;

namespace Rnd {
class Blur;
class Text;
class TransAnim;
} // namespace Rnd

/**
 * One animated text message of the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the three objects it resolves, `<name>.blur`, `<name>.txt`, and
 * `<name>.tnm`. HudTrack embeds one at `+0x28` under the name `<layout> textmsg<n>`, and
 * HudWinMessage embeds one under `HUD winmsg`.
 *
 * A message fades in over 250 units of SetFrame()'s time, stays for the time Show() gives it, and
 * fades out over 250 more. Show() scales the text's font from its size at construction, and the
 * destructor restores that size.
 */
class HudTextMessage {
public:
    /**
     * Resolve the message's objects and hide it.
     *
     * Records the text's font and its size at construction, hides the text, and shows the blur
     * when one exists.
     *
     * @param name The object name the three suffixes are appended to.
     * @ghidraAddress 0x00416dd8
     */
    HudTextMessage(const HxStr &name);

    /**
     * Restore the font's size.
     *
     * HudTrack's destructor and Overlay's destructor inline the body, and the out-of-line copy has
     * no caller.
     *
     * @ghidraAddress 0x00429aa8
     */
    ~HudTextMessage() {
        mFont->SetSize(mFontSize);
    }

    /**
     * Start showing one text unless a message is already active.
     *
     * Scales the font from its original size, sets and shows the text, rewinds the animation, and
     * marks the start time as pending until the next SetFrame(). The title is inferred.
     *
     * @param text The text to show.
     * @param flScale The font scale relative to the original size.
     * @param flHold How long the message stays between its fades.
     * @ghidraAddress 0x00429b58
     */
    void Show(const HxStr &text, float flScale, float flHold);

    /**
     * Hide the text at once and mark the message inactive.
     *
     * The title is inferred.
     *
     * @ghidraAddress 0x00429af8
     */
    void Hide();

    /**
     * Advance the fade animation, hiding the text once it has faded out.
     *
     * @param flTime The current time. The first call after Show() records it as the start.
     * @ghidraAddress 0x00429c20
     */
    void SetFrame(float flTime);

private:
    Rnd::Blur *mBlur; // `<name>.blur`
    Rnd::Font *mFont; // The text's font.

public:
    /**
     * The text, `<name>.txt`. +0x08
     *
     * Public because HudWinMessage::Draw() at `0x0042a830` draws it directly, and the image has no
     * accessor for it.
     */
    Rnd::Text *mText;

private:
    Rnd::TransAnim *mAnim; // `<name>.tnm`
    // When the message started. 0 marks it idle and 1e9 marks a start not yet recorded.
    float mStart;
    // The font size at construction. The destructor restores it.
    float mFontSize;
    // How long the message stays between its fades.
    float mHold;

public:
    /**
     * Non-zero while a message is active. +0x1c
     *
     * Show() does nothing while it is set, and Hide() and SetFrame() clear it. Show() never sets
     * it. Overlay's handler at `0x0041e020` sets it directly after an inlined Show() whenever the
     * text reports itself showing, and the image has no accessor for it.
     */
    int mActive;
};
