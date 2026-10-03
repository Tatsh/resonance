#pragma once

namespace Rnd {
class Animatable;
} // namespace Rnd

/**
 * One run of an animation between two frames, played against a running time.
 *
 * The class is not polymorphic and emits no RTTI. Its name comes from the debugging symbols of the
 * North American demo release. The demo's constructor and SetAnim() have the same instructions as
 * this class's. HudPoints and several of the tunnel's components embed it by value.
 *
 * Play() records the two frames in the animation's own numbering and arms the run. Each Update()
 * then advances the animation one frame per unit of time from the first frame, and parks it on the
 * last frame once the run passes it.
 */
class AnimHelper {
public:
    /**
     * Start idle with no animation.
     *
     * @ghidraAddress NTSC-U/C: 0x00411a18
     * @ghidraAddress PAL: 0x0044b4e0
     */
    AnimHelper();

    /**
     * Attach the animation and rewind it to frame 0.
     *
     * @param pAnim The animation to play.
     * @ghidraAddress NTSC-U/C: 0x00411a30
     * @ghidraAddress PAL: 0x0044b4f8
     */
    void SetAnim(Rnd::Animatable *pAnim);

    /**
     * Arm a run between two frames.
     *
     * Each frame is mapped back through the animation's filters first. The run starts at the next
     * Update().
     *
     * @param flFrom The first frame.
     * @param flTo The last frame.
     * @ghidraAddress NTSC-U/C: 0x00411a58
     * @ghidraAddress PAL: 0x0044b520
     */
    void Play(float flFrom, float flTo);

    /**
     * Advance a run in progress.
     *
     * @param flTime The current time. The first call after Play() records it as the start.
     * @return 1 while the run continues, and 0 when idle or once the run has finished.
     * @ghidraAddress NTSC-U/C: 0x00411ab8
     * @ghidraAddress PAL: 0x0044b580
     */
    int Update(float flTime);

private:
    // When the run started. -1 marks the range idle and 0 marks a run not yet started.
    float mStart;           // +0x00
    float mFrom;            // +0x04
    float mTo;              // +0x08
    Rnd::Animatable *mAnim; // +0x0c
};
