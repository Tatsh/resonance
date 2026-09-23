#pragma once

struct Color;
struct Plane;
struct Vector3;
namespace Rnd {
class Mat;
class String;
class View;
} // namespace Rnd

/**
 * Straight segments of one lane of one tunnel slice, drawn as line pairs of one ribbon.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the durable-gem trails that allocate it and from the one lane and
 * slice row it draws. DurGemTrails allocates one per lane per row with the untagged scalar
 * allocator (0x10 bytes) and indexes them by row and then by lane.
 *
 * Every member is public because DurGemTrails reads mString directly in AddSegment(), StartStrip(),
 * and Update().
 */
class DurGemRowString {
public:
    /**
     * Create the ribbon, set it to draw line pairs, and add it to pView.
     *
     * The ribbon takes the next DurGemTrails::NewStringName() name. mRow starts at -100.
     *
     * @param nLane The lane the segments follow.
     * @param pMat The material the ribbon draws with.
     * @param pView The view that draws the ribbon.
     * @param flWidth The width of the ribbon.
     * @ghidraAddress 0x00432830
     */
    DurGemRowString(int nLane, Rnd::Mat *pMat, Rnd::View *pView, float flWidth);

    /**
     * Delete the ribbon.
     *
     * The destructor of DurGemTrails inlines it. The out-of-line copy is the deleting form, which
     * frees the object only when bit 0 of the flag is set, and it has no caller. Its place at the
     * head of this unit, ahead of AddLine(), is what assigns it to this class rather than to
     * DurGemStrip, whose destructor has the same body.
     *
     * @ghidraAddress 0x00436ee0
     */
    ~DurGemRowString();

    /**
     * Append one straight segment in color between two tunnel positions.
     *
     * A segment whose start frame falls in another slice row than mRow first moves the string to
     * that row and discards its segments. DurGemTrails::AddSegment() inlines the body.
     *
     * @param color The colour of both ends.
     * @param flStartFrame The tunnel frame of the start.
     * @param flStartBlend The position across the lane at the start.
     * @param flEndFrame The tunnel frame of the end.
     * @param flEndBlend The position across the lane at the end.
     * @ghidraAddress 0x00436f48
     */
    void AddLine(const Color &color,
                 float flStartFrame,
                 float flStartBlend,
                 float flEndFrame,
                 float flEndBlend);

    /**
     * Discard the segments and hide the ribbon.
     *
     * mRow becomes -1000. DurGemTrails::EndTrail() inlines the body.
     *
     * @ghidraAddress 0x004370b8
     */
    void Clear();

    /**
     * Show the ribbon when it belongs to slice row nRow and hide it otherwise.
     *
     * DurGemTrails::Update() inlines the body.
     *
     * @param nRow The slice row being drawn.
     * @return The point count of the ribbon when shown, otherwise 0.
     * @ghidraAddress 0x004370f8
     */
    int Show(int nRow);

    /**
     * Hide the ribbon without discarding the segments.
     *
     * The shipped program does not call it. A routine with the same body at `0x0042a9e8`, in the
     * head-up display unit, remains unplaced, because nothing there identifies the class it belongs
     * to. The title is inferred.
     *
     * @ghidraAddress 0x00437180
     */
    void Hide();

    /**
     * Find where a segment crosses plane.
     *
     * Only a crossing between the two ends of a segment counts. DurGemTrails::Update() inlines the
     * body.
     *
     * @param plane The plane to test against.
     * @param pOut Receives the crossing point.
     * @return True when a segment crosses plane.
     * @ghidraAddress 0x004371b0
     */
    bool FindCrossing(const Plane &plane, Vector3 *pOut);

    Rnd::String *mString; /*!< The ribbon. */
    int mLane;            /*!< The lane the segments follow. */
    int mRow;             /*!< The slice row of the segments, -100 before the first. */
    int mCount;           /*!< The number of segments, two ribbon points each. */
};
