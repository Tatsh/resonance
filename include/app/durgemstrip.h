#pragma once

struct Color;
struct Vector3;
namespace Rnd {
class Mat;
class View;
class String;
} // namespace Rnd

/**
 * Live trail of one held durable gem, a two-point ribbon that grows with the playhead.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the growing two-point strip it draws. DurGemTrails allocates six
 * with the untagged scalar allocator (0x1c bytes) and hands each new trail to the first free one.
 *
 * The trail runs from mStart to mEnd along lane mLane, at most 720 frames long. Once stopped, it
 * stays in place until the playhead passes mEnd by 480 frames and the strip is freed.
 */
class DurGemStrip {
public:
    /**
     * Create the free strip's hidden two-point ribbon and add it to pView.
     *
     * The ribbon takes the next DurGemTrails::NewStringName() name.
     *
     * @param pView The view that draws the ribbon.
     * @ghidraAddress 0x004329e8
     */
    explicit DurGemStrip(Rnd::View *pView);

    /**
     * Delete the ribbon.
     *
     * The binary has no out-of-line copy. The destructor of DurGemTrails inlines it.
     */
    ~DurGemStrip();

    /**
     * Claim the strip for trail nId when it is free.
     *
     * Both ends start at flFrame. DurGemTrails::StartStrip() inlines the body.
     *
     * @param nLane The lane the trail follows.
     * @param color The colour of both ends.
     * @param nId The identifier of the trail.
     * @param flFrame The tunnel frame the trail starts at.
     * @param flBlend The position across the lane.
     * @param flWidth The width of the ribbon.
     * @param pMat The material the ribbon draws with.
     * @return True when the strip was free and now draws the trail.
     * @ghidraAddress 0x00437340
     */
    bool Start(int nLane,
               const Color &color,
               int nId,
               float flFrame,
               float flBlend,
               float flWidth,
               Rnd::Mat *pMat);

    /**
     * Extend the head of the trail to flFrame and free the strip once the trail is behind.
     *
     * A stopped trail does not extend. A trail longer than 720 frames moves its tail up behind the
     * head. The strip is hidden and freed once flFrame passes mEnd by more than 480 frames.
     *
     * @param flFrame The current tunnel frame.
     * @ghidraAddress 0x00432b90
     */
    void Update(float flFrame);

    /**
     * Extend and then stop the trail when it is trail nId.
     *
     * DurGemTrails::StopStrip() inlines the body.
     *
     * @param nId The identifier of the trail to stop.
     * @param flFrame The tunnel frame the trail ends at.
     * @return True when the strip drew trail nId.
     * @ghidraAddress 0x00437480
     */
    bool Stop(int nId, float flFrame);

    /**
     * Report the lane position at flFrame when flFrame lies on the trail.
     *
     * DurGemTrails::Update() inlines the body.
     *
     * @param flFrame The tunnel frame to test.
     * @param pOut Receives the position.
     * @return True when the strip is in use and flFrame lies between mStart and mEnd.
     * @ghidraAddress 0x004374c0
     */
    bool GetHeadPos(float flFrame, Vector3 *pOut);

private:
    Rnd::String *mString; // The two-point ribbon.
    int mId;              // The identifier of the trail, kFreeId when free.
    int mStopped;         // Non-zero once Stop() has ended the trail.
    int mLane;            // The lane the trail follows.
    float mBlend;         // The position across the lane.
    float mStart;         // The tunnel frame of the tail.
    float mEnd;           // The tunnel frame of the head.
};
