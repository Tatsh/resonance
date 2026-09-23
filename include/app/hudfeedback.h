#pragma once

/**
 * Frame feedback the head-up display runs before the song starts.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * resolves no object, and its name is inferred from the GfxDevice frame feedback it drives. The
 * head-up display panel embeds one at `+0x160`.
 *
 * Before song position -5500 the feedback runs at a fixed strength. Between -5500 and -2000 its
 * alpha and its texture inset fall to zero, and from -2000 onwards, and whenever the song position
 * stands still, the feedback is switched off.
 */
class HudFeedback {
public:
    /**
     * Drive the frame feedback for one song position.
     *
     * Does nothing at a positive position.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress 0x0041bc50
     */
    void SetFrame(float flFrame);

private:
    // The song position the last call received.
    float mLastFrame;
};
