#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "msg/message.h"

/**
 * Player of the phrases one track is divided into.
 *
 * `12PhrasePlayer` in the RTTI descriptor at `0x008eff30`, over MsgSource at offset 0 and MsgSink
 * at offset 20. The primary table is at `0x007e3120` with four entries and the MsgSink subobject
 * table at `0x007e30f8`. MsgSource coming first is why every upcast to MsgSink in the stage classes
 * adjusts by `+0x14` and guards the adjustment against a null pointer. The object is 0x30 bytes.
 *
 * The class is not reconstructed. Only the surface the stage classes use is declared, so that they
 * compile against the real type. Its constructor is at `0x001c17d8`.
 */
class PhrasePlayer : public MsgSource, public MsgSink {
public:
    /**
     * @ghidraAddress 0x001c2658
     */
    virtual ~PhrasePlayer();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001c2928
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Play the phrase of one bar.
     *
     * PhraseMgr::OnCommand() calls it once a bar. The body is not written. It forwards the step
     * value PhraseMgr::GetStepValue() reports to the object at `+0x28` when there is one, and for
     * a bar past `+0x2c` with a phrase it dispatches on the track mode at `+0x24` through the jump
     * table at `0x007e3070`.
     *
     * @param nBar The bar.
     * @ghidraAddress 0x001c1860
     */
    void PlayBar(int nBar);

    /**
     * Play a bar again from an offset, already some ticks under way.
     *
     * PhraseMgr::ReplayBar() is the recovered caller. The body is not written.
     *
     * @param nBar The bar.
     * @param nOffset The offset within the bar, in MIDI ticks.
     * @param nElapsed The ticks since the bar started.
     * @ghidraAddress 0x001c2818
     */
    void PlayBarAt(int nBar, int nOffset, int nElapsed);
};
