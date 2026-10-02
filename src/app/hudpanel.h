#pragma once

#include "app/hudanalogstick.h"
#include "app/hudanimramp.h"
#include "app/hudfeedback.h"
#include "app/hudgenmessage.h"
#include "app/hudhighlight.h"
#include "app/hudletterbox.h"
#include "app/hudposition.h"
#include "app/hudscorepulse.h"
#include "app/hudscreenflash.h"
#include "app/hudtcgroup.h"
#include "app/hudwinmessage.h"

/**
 * The parts of the head-up display that belong to the whole screen rather than to one player.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred. Overlay allocates one with the untagged scalar allocator (0x164 bytes)
 * and records it at its `+0x04`.
 *
 * The class declares no destructor. Overlay's destructor inlines the implicit one
 * (~HudTextMessage() for the win message and ~HudPosition() at `0x0041ac18`) and then frees the
 * block.
 *
 * Every member is public because Overlay's handlers drive the parts directly.
 */
class HudPanel {
public:
    /**
     * Build every part.
     *
     * The song position bar is built over the play map Globals::GetPlayMap() reports, the assembly
     * animation `<layout> assembly.view` ramps from frame 0 to frame 100 over 480 units, and the
     * label swap animation `HUD1 label swap.tnm` ramps from frame 100 to frame 200 over 480 units.
     *
     * @ghidraAddress 0x0041c3a0
     */
    HudPanel();

    /**
     * Advance every animated part.
     *
     * The song position bar, the highlight box, the letterbox, and the frame feedback follow the
     * song position, and the two animation ramps and the win message follow the time. The screen
     * flash reads the free-running clock.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @param flTime The time.
     * @ghidraAddress 0x0041c7d8
     */
    void SetFrame(float flFrame, float flTime);

    HudPosition mPosition;       /*!< The song position bar. +0x00 */
    HudScreenFlash mScreenFlash; /*!< The full-screen fade. +0x40 */
    HudGenMessage mMessage;      /*!< `HUD genmsg.txt`. +0x54 */
    HudAnimRamp mAssembly;       /*!< The assembly animation. +0x58 */
    HudAnimRamp mLabelSwap;      /*!< The label swap animation. +0x74 */
    HudHighlight mHighlight;     /*!< The highlight box. +0x90 */
    HudAnalogStick mAnalogStick; /*!< The analog stick prompt. +0x104 */
    HudTcGroup mTcGroup;         /*!< `tc_hi_group.view`. +0x110 */
    HudScorePulse mScorePulse;   /*!< The pulse over the leader's score. +0x114 */
    HudLetterbox mLetterbox;     /*!< The letterbox bars. +0x118 */
    HudWinMessage mWinMessage;   /*!< The closing sequence. +0x134 */
    HudFeedback mFeedback;       /*!< The frame feedback before the song. +0x160 */
};
