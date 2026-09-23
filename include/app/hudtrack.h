#pragma once

#include "app/hudcountdown.h"
#include "app/hudeffects.h"
#include "app/hudenergy.h"
#include "app/hudloop.h"
#include "app/hudpoints.h"
#include "app/hudpowerup.h"
#include "app/hudtextmessage.h"
#include "app/hudtracklabel.h"

class Player;

/**
 * One player's track display on the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. Overlay allocates one per world player that has
 * a slot, with the untagged scalar allocator (0xf4 bytes, the size its constructor requests at
 * `0x0041cf50`), and stores the pointers in its vector at `+0x08`. No descriptor, tag, or file path
 * identifies the class, and its name is inferred from the eight parts it aggregates.
 *
 * The class declares no destructor. The routine at `0x0042aa18` that Overlay's destructor calls on
 * each element is the compiler's deleting destructor, which runs the effect lamps' vector teardown
 * and ~HudTextMessage() and then the scalar free.
 *
 * Every member is public because Overlay's handlers access the parts directly. They look a
 * display up by mPlayer, set the track name through mTrackLabel, record the track in mUnknowne4,
 * and drive mPoints, mEffects, and the words at `+0xe0` and `+0xec`.
 *
 * The constructor's body is not written. It needs the Globals accessor at `0x00118da0`, whose
 * result supplies the countdown's target bar through its slot 9.
 */
class HudTrack {
public:
    /**
     * Build the display for one player.
     *
     * @param pPlayer The player the display shows.
     * @param nIndex The display number, counted over the players with a slot from 0.
     * @ghidraAddress 0x0041bec8
     */
    HudTrack(Player *pPlayer, int nIndex);

    /**
     * Advance every animated part.
     *
     * The energy bar and the countdown follow the song position, and the text message and the
     * points readout follow the time.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @param flTime The time the text message and the points readout run against.
     * @ghidraAddress 0x0042ab08
     */
    void SetFrame(float flFrame, float flTime);

    HudEnergy mEnergy;           /*!< The shared energy bar. +0x00 */
    HudPowerup mPowerup;         /*!< The powerup indicator. +0x0c */
    HudTextMessage mTextMessage; /*!< `<layout> textmsg<n>`. +0x28 */
    HudLoop mLoop;               /*!< The loop indicator. +0x48 */
    HudTrackLabel mTrackLabel;   /*!< The track name label. +0x50 */
    HudEffects mEffects;         /*!< The effect lamps. +0x58 */
    HudPoints mPoints;           /*!< The points readout. +0x78 */
    HudCountdown mCountdown;     /*!< The bar countdown. +0xc4 */
    int mUnknowne0;              /*!< Word Overlay's handler at `0x0041fed8` counts. +0xe0 */
    int mUnknowne4;              /*!< The track the player selected. +0xe4 */
    int mUnknowne8;              /*!< The constructor does not write it. +0xe8 */
    int mUnknownec;              /*!< Word Overlay's handler at `0x0041eda8` writes. +0xec */
    Player *mPlayer;             /*!< The player the display shows. +0xf0 */
};
