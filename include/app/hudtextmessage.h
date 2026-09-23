#pragma once

class HxStr;

namespace Rnd {
class Blur;
class Font;
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
 *
 * Three bodies are not written. The constructor, the destructor, and Show() all need the Rnd::Font
 * size setter at `0x004d0648`, and Show() also clears a list Rnd::Blur declares private.
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
     * HudTrack's destructor and Overlay's destructor inline the body, and the out-of-line copy at
     * `0x00429aa8` has no caller.
     */
    ~HudTextMessage();

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
    Rnd::Blur *mUnknown00;      // +0x00
    Rnd::Font *mUnknown04;      // +0x04 The text's font.
    Rnd::Text *mUnknown08;      // +0x08
    Rnd::TransAnim *mUnknown0c; // +0x0c
    // When the message started. 0 marks it idle and 1e9 marks a start not yet recorded.
    float mUnknown10; // +0x10
    // The font size at construction, which the destructor restores.
    float mUnknown14; // +0x14
    // How long the message stays between its fades.
    float mUnknown18; // +0x18

public:
    /**
     * Non-zero while a message is active. +0x1c
     *
     * Show() does nothing while it is set, and Hide() and SetFrame() clear it. Show() never sets
     * it. Overlay's handler at `0x0041e020` sets it directly after an inlined Show() whenever the
     * text reports itself showing, and the image has no accessor for it.
     */
    int mUnknown1c;
};
