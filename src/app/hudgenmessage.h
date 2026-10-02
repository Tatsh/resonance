#pragma once

class HxStr;

namespace Rnd {
class Text;
} // namespace Rnd

/**
 * One plain text message of the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from `HUD genmsg.txt`, the object both of its instances resolve. The
 * head-up display panel embeds one at `+0x54`, and HudWinMessage embeds one at `+0x28`.
 *
 * Unlike HudTextMessage, the message has no animation and no timing. It shows a text until it is
 * hidden.
 */
class HudGenMessage {
public:
    /**
     * Resolve the text and hide it.
     *
     * The panel and HudWinMessage both inline the body, and this copy has no caller.
     *
     * @param name The object name of the text.
     * @ghidraAddress 0x0042a6d8
     */
    HudGenMessage(const HxStr &name);

    /**
     * Show one text.
     *
     * The out-of-line copy has no caller. The title is inferred.
     *
     * @param text The text.
     * @ghidraAddress 0x0042a768
     */
    void Show(const HxStr &text);

    /**
     * Hide the text.
     *
     * The out-of-line copy has no caller. The title is inferred.
     *
     * @ghidraAddress 0x0042a7c0
     */
    void Hide();

    /**
     * The text. +0x00
     *
     * Public because HudWinMessage's draw at `0x0042a830` draws it directly, and the image has no
     * accessor for it.
     */
    Rnd::Text *mText;
};
