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
 * Curved durable-gem segment of one lane, drawn as one continuous ribbon.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the curved path it draws. DurGemTrails stores the records by value
 * in a list sorted by mRow, the 0xc-byte value of a 0x18-byte list node.
 *
 * The copy is the compiler's member-wise copy. DurGemTrails::AddSegment() inserts a default record
 * and then calls Init() on the copy in the list.
 */
class DurGemCurve {
public:
    /**
     * Construct an empty record with no ribbon.
     *
     * Only mString is set. The binary has no out-of-line copy.
     */
    DurGemCurve() : mString(nullptr) {
    }

    /**
     * Delete the ribbon.
     *
     * @ghidraAddress 0x004375a8
     */
    ~DurGemCurve();

    /**
     * Create the ribbon through the set points of pPoints and add it to pView hidden.
     *
     * A point at exactly the origin marks a point that subdivision did not need and is skipped.
     *
     * @param pView The view that draws the ribbon.
     * @param nLane The lane the curve follows.
     * @param nRow The slice row of the curve.
     * @param pPoints The nine subdivision points, DurGemTrails::kSegmentPointCount of them.
     * @param color The colour of every point.
     * @param pMat The material the ribbon draws with.
     * @param flWidth The width of the ribbon.
     * @ghidraAddress 0x00432cf0
     */
    void Init(Rnd::View *pView,
              int nLane,
              int nRow,
              const Vector3 *pPoints,
              const Color &color,
              Rnd::Mat *pMat,
              float flWidth);

    /**
     * Show the ribbon.
     *
     * DurGemTrails::Update() inlines the body.
     *
     * @return The point count of the ribbon.
     * @ghidraAddress 0x00437610
     */
    int Show();

    /**
     * Hide the ribbon.
     *
     * DurGemTrails::Update() inlines the body.
     *
     * @ghidraAddress 0x00437670
     */
    void Hide();

    /**
     * Find where the ribbon crosses plane.
     *
     * Consecutive points form the segments, and the result of IntersectSegmentWithPlane() decides
     * the crossing. DurGemTrails::Update() inlines the body.
     *
     * @param plane The plane to test against.
     * @param pOut Receives the crossing point.
     * @return True when a segment crosses plane.
     * @ghidraAddress 0x004376a0
     */
    bool FindCrossing(const Plane &plane, Vector3 *pOut);

    int mLane;            /*!< The lane the curve follows. */
    int mRow;             /*!< The slice row of the curve, the list's sort key. */
    Rnd::String *mString; /*!< The ribbon, or null before Init(). */
};
