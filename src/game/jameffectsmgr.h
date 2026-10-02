#pragma once

#include <vector>

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "gs/effector.h"
#include "msg/jameffectmsg.h"
#include "msg/message.h"

class PhraseMgr;
class PlayMap;

/**
 * Manager of the jam effects a track applies to its voices.
 *
 * `13JamEffectsMgr` in the RTTI descriptor at `0x008f0490`, over MsgSink at offset 0 and MsgSource
 * at offset 4. The primary table is at `0x007df298` and the MsgSource subobject table at
 * `0x007df270`, which adjusts `this` by `-4`. The object is 0x34 bytes, which the tagged
 * allocations in PitchingSTG and VoxingSTG both measure. Both build one only when the game manager
 * reports play mode 2, and both then hand it to the phrase player.
 *
 * The constructor builds one Effector for every type configuration code 0x389 lists, each wired
 * to the sink the stage supplies. A JamEffectMsg toggles one effect for one step of the phrase
 * manager, and every bar the phrase player applies the step's mask to all of them.
 */
class JamEffectsMgr : public MsgSink, public MsgSource {
public:
    /**
     * PitchingSTG's constructor at `0x001c46e8` supplies every argument.
     *
     * @param nTrack The track, the stage's first word.
     * @param nChannel The MIDI channel of the track, from TrackData.
     * @param pPlayMap The play map, from Globals::GetPlayMap().
     * @param pPhraseMgr The phrase manager whose step values the effects follow.
     * @param pSink The sink every effect sends its MIDI to, the stage's synthesiser.
     * @ghidraAddress 0x001a5020
     */
    JamEffectsMgr(int nTrack,
                  unsigned char nChannel,
                  PlayMap *pPlayMap,
                  PhraseMgr *pPhraseMgr,
                  MsgSink *pSink);

    /**
     * Delete every effect.
     *
     * @ghidraAddress 0x001a5378
     */
    virtual ~JamEffectsMgr();

    /**
     * Act on a message.
     *
     * Slot 3. A JamEffectMsg goes to PostRemixFxMsg(), and every other message is discarded.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001a63d0
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Switch every effect on or off from one bit of a mask.
     *
     * PhrasePlayer::PlayBar() passes the step value of each bar. For each effect, the bit its
     * Type() reports, taken modulo 64, decides the argument to its Enable(). The bit test goes
     * through a `std::bitset` reference built on the stack. The title is inferred.
     *
     * @param nMask The step's mask, one bit per effect type.
     * @ghidraAddress 0x001a56d8
     */
    void ApplyStepMask(long long nMask);

private:
    // For a message on this track, flips the effect's bit in the step value of the message's bar,
    // enables or disables the effect to match, sends an InvalidateTrackMsg for the whole song to
    // the phrase manager, and sends a RemixFXMsg. An effect type from 5 through 10 also marks the
    // world's statistics.
    // 0x001a54d8
    void PostRemixFxMsg(JamEffectMsg *pMsg);

    // Returns the first effect whose Type() is nType, or null.
    // 0x001a62d8
    Effector *FindEffector(int nType);

    // Passes one flag to every effect's Enable(). The image has no caller.
    // 0x001a6350
    void EnableAll(int bEnabled);

    PlayMap *mPlayMap;                  // +0x18, not read by any recovered routine
    PhraseMgr *mPhraseMgr;              // +0x1c
    int mTrack;                         // +0x20
    unsigned char mChannel;             // +0x24
    std::vector<Effector *> mEffectors; // +0x28
};
