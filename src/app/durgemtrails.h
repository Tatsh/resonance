#pragma once

#include <list>
#include <vector>

#include "app/durgemcurve.h"
#include "os/hxstr.h"

class AppTunnel;
class DurGemRowString;
class DurGemStrip;
struct Color;
struct Vector3;
namespace Rnd {
class Mat;
class View;
} // namespace Rnd

/**
 * Ribbons that trace durable gems along the tunnel lanes.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the DurGemMsg handlers of AppTunnel that drive it and from the
 * trails it draws. AppTunnel allocates one (0x30 bytes) and stores it at `+0x88`.
 *
 * Straight segments go to a DurGemRowString per lane and slice row, curved segments to a
 * DurGemCurve in a list sorted by row, and live trails to one of six DurGemStrip records. Every
 * ribbon is added to the view "tnl strings". Update() shows the rows from the playhead onward, up
 * to mMaxPoints ribbon points, and reports where the ribbons cross the playhead plane.
 */
class DurGemTrails {
public:
    /** The number of subdivision points AddSegment() places on one segment. */
    enum { kSegmentPointCount = 9 };

    /**
     * Allocate the row table and six strips, and resolve the view "tnl strings".
     *
     * The table has room for eight lanes over eight slice rows, and every entry starts null
     * until CreateLane() fills it.
     *
     * @param pTunnel The owner and receiver of the crossing points Update() finds.
     * @param nMaxPoints The most ribbon points Update() shows at once.
     * @ghidraAddress 0x00432f60
     */
    DurGemTrails(AppTunnel *pTunnel, int nMaxPoints);

    /**
     * Delete the row strings and the strips.
     *
     * The curve list and the two vectors are torn down by their compiler-generated destructors.
     *
     * @ghidraAddress 0x00433320
     */
    ~DurGemTrails();

    /**
     * Create the row strings of lane nLane for every slice row.
     *
     * @param flWidth The width of each ribbon.
     * @param pMat The material each ribbon draws with.
     * @param nLane The lane to fill.
     * @ghidraAddress 0x004377f0
     */
    void CreateLane(float flWidth, int nLane, Rnd::Mat *pMat);

    /**
     * Add a segment of lane nLane between two tunnel positions.
     *
     * The segment is subdivided until every midpoint lies within 0.01 of the chord. A segment
     * that needs no midpoint goes to the row string of its start row as a straight line.
     * Otherwise a DurGemCurve through the subdivision points is inserted before the first curve
     * of the same or a later row, with the material and width of that row string.
     *
     * @param nLane The lane the segment follows.
     * @param color The colour of the segment.
     * @param flStartFrame The tunnel frame of the start.
     * @param flStartBlend The position across the lane at the start.
     * @param flEndFrame The tunnel frame of the end.
     * @param flEndBlend The position across the lane at the end.
     * @ghidraAddress 0x004337d8
     */
    void AddSegment(int nLane,
                    const Color &color,
                    float flStartFrame,
                    float flStartBlend,
                    float flEndFrame,
                    float flEndBlend);

    /**
     * Clear the row string of lane nLane in slice row nRow and erase the curves of that lane and
     * row.
     *
     * @param nLane The lane to clear.
     * @param nRow The slice row to clear.
     * @ghidraAddress 0x00433c40
     */
    void EndTrail(int nLane, int nRow);

    /**
     * Hand a new live trail to the first free strip.
     *
     * The strip takes the material and width of the row-0 string of nLane. The trail is dropped
     * when every strip is in use.
     *
     * @param nLane The lane the trail follows.
     * @param color The colour of the trail.
     * @param nId The identifier of the trail.
     * @param flFrame The tunnel frame the trail starts at.
     * @param flBlend The position across the lane.
     * @ghidraAddress 0x00433db0
     */
    void StartStrip(int nLane, const Color &color, int nId, float flFrame, float flBlend);

    /**
     * Stop the live trail nId at flFrame.
     *
     * @param nId The identifier of the trail.
     * @param flFrame The tunnel frame the trail ends at.
     * @ghidraAddress 0x00437a60
     */
    void StopStrip(int nId, float flFrame);

    /**
     * Show the ribbons of the slice rows from 480 frames behind flFrame onward and advance the
     * strips.
     *
     * Curves of rows behind the first shown row are erased. Ribbons past mMaxPoints points are
     * hidden. Each crossing of the playhead plane by a ribbon of the first two rows, and the head
     * of each live strip, goes to mTunnel.
     *
     * @param flFrame The current tunnel frame.
     * @ghidraAddress 0x00433f80
     */
    void Update(float flFrame);

    /**
     * Increment g_nDurGemStringCount and format the next ribbon name.
     *
     * The three ribbon owners inline the body.
     *
     * @return The new count formatted with the literal at `0x0081a768`, four digits in angle
     * brackets after a fixed prefix.
     * @ghidraAddress 0x00436e38
     */
    static HxStr NewStringName();

private:
    // 0x00437af8
    // The row string of lane nLane in slice row nRow, the row taken modulo mRowCount.
    DurGemRowString *GetRowString(int nLane, int nRow);

    // 0x00433530
    // Place the midpoint of pPoints[nFirst] and pPoints[nLast] and recurse into both
    // halves while it lies off the chord.
    static void SubdivideSegment(int nLane,
                                 Vector3 *pPoints,
                                 int nFirst,
                                 int nLast,
                                 float flStartFrame,
                                 float flStartBlend,
                                 float flEndFrame,
                                 float flEndBlend);

    // 0x004378e0
    // Report the midpoint frame, blend, and position of a segment, and whether the
    // position lies off the chord of first and last. SubdivideSegment() inlines the body.
    static bool NeedsSubdivision(int nLane,
                                 const Vector3 &first,
                                 const Vector3 &last,
                                 float *pMidFrame,
                                 float *pMidBlend,
                                 Vector3 *pMidPoint,
                                 float flStartFrame,
                                 float flStartBlend,
                                 float flEndFrame,
                                 float flEndBlend);

    // 0x00436e88
    // The first curve of row nRow or later. AddSegment() and EndTrail() inline the
    // body.
    static std::list<DurGemCurve>::iterator FindFirstCurve(std::list<DurGemCurve> &curves,
                                                           int nRow);

    // A signed remainder moved into [0, nCount).
    static int WrapIndex(int nIndex, int nCount) {
        const int nRemainder = nIndex % nCount;
        return nRemainder > -1 ? nRemainder : nRemainder + nCount;
    }

    int mLaneCount;
    int mRowCount;
    int mMaxPoints;
    std::vector<DurGemRowString *> mRows; // Row-major, mRowCount rows of mLaneCount lanes.
    std::vector<DurGemStrip *> mStrips;
    std::list<DurGemCurve> mCurves; // Sorted by DurGemCurve::mRow.
    Rnd::View *mView;               // `tnl strings`
    AppTunnel *mTunnel;
};

/**
 * Number of ribbon names DurGemTrails::NewStringName() and its inlined copies have issued.
 *
 * @ghidraAddress 0x006e3440
 */
extern int g_nDurGemStringCount;
