#pragma once

#include "app/hudgenmessage.h"
#include "app/hudtextmessage.h"

/**
 * Closing sequence of the head-up display after a campaign is won.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from `HUD winmsg`, the message it animates. The head-up display panel
 * embeds one at `+0x134`.
 *
 * Overlay's WinMsg handler starts the sequence by setting mState to kStateStart. SetFrame() then
 * sets up the frame feedback, and shows three messages in turn, a congratulation for the session's
 * difficulty, then two credits, and finally the prompt to exit.
 */
class HudWinMessage {
public:
    /** Steps of the closing sequence. */
    enum State {
        kStateIdle = 0,     /*!< Nothing shows. */
        kStateStart = 1,    /*!< The next SetFrame() shows the congratulation. */
        kStateCongrats = 2, /*!< The congratulation shows. */
        kStateTeam = 3,     /*!< The first credit shows. */
        kStateThanks = 4,   /*!< The second credit shows. */
        kStatePrompt = 5,   /*!< The prompt to exit shows. */
    };

    /**
     * Resolve the two messages and start idle.
     *
     * @ghidraAddress 0x0041b4c8
     */
    HudWinMessage();

    /**
     * Advance the message animation and step the sequence.
     *
     * The congratulation shows at 2.5 times the message font size, the first credit 10000 units of
     * time later at twice the size, the second credit 10000 units after that at 1.7 times the size,
     * and the prompt 8000 units after that. Each message stays for 8000 units between its fades.
     *
     * @param flTime The current time.
     * @ghidraAddress 0x0041b658
     */
    void SetFrame(float flTime);

    /**
     * Hide the prompt to exit.
     *
     * The out-of-line copy has no caller. The title is inferred.
     *
     * @ghidraAddress 0x0042a800
     */
    void HidePrompt();

private:
    // When the current step started. Starts at 1e9.
    float mStart;

public:
    /**
     * The step of the sequence. +0x04
     *
     * Public because Overlay's WinMsg handler at `0x0041e020` writes kStateStart into it directly,
     * and Overlay::Draw() tests it, and the image has no accessor for it.
     */
    int mState;

private:
    HudTextMessage mMessage; // `HUD winmsg`
    HudGenMessage mPrompt;   // `HUD genmsg.txt`
};
