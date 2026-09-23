#pragma once

#include "app/tnlemitter.h"

class HxStr;
struct Color;
namespace Rnd {
class TransAnim;
class View;
} // namespace Rnd

/**
 * Coloured particle fire that travels down the tunnel path from one lane.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the name prefixes AppTunnel builds these with, "fire%d",
 * "firefs%d", and "firemult%d", and from the shared path "fx.path".
 *
 * Each object resolves "<name>.view" and "<name>.ps". The "fire%d" objects, built with an index of
 * -1, also resolve "<name>a.ps". Start() colours the systems, turns them to face a lane, and runs
 * "fx.path" forward at four path frames per song frame until the path passes its end frame.
 *
 * AppTunnel allocates these in three groups, 0x70 bytes each, and stores them in its vector at
 * `+0x24`. The group built with the name "fire%d" passes -1, "firefs%d" passes -2, and
 * "firemult%d" passes the loop index.
 */
class TnlFireFX {
public:
    /**
     * Resolve the path, the view, and the systems of one fire.
     *
     * @param name The name prefix.
     * @param nIndex The index Start() must be given, -1 when the object has a second system.
     * @ghidraAddress 0x0043d368
     */
    TnlFireFX(const HxStr &name, int nIndex);

    /**
     * Start the fire if it is idle and nIndex selects it.
     *
     * Each system spawns white and fades from its colour at full alpha to the same colour at zero
     * alpha. Both systems are turned by `-nSlot` eighths of a turn about Y, and each recorded
     * force is turned by that rotation composed with the path basis at flPathStart. The path then
     * runs from flPathStart and the fire ends once it passes flPathEnd.
     *
     * @param flPathStart The first path frame.
     * @param nIndex The index this call is for.
     * @param nSlot The lane the fire faces.
     * @param color The colour of the first system.
     * @param altColor The colour of the second system.
     * @param flPathEnd The path frame that ends the fire.
     * @return 1 when the fire started, 0 when it was busy or nIndex did not match.
     * @ghidraAddress 0x0043d8c0
     */
    int Start(float flPathStart,
              int nIndex,
              int nSlot,
              const Color &color,
              const Color &altColor,
              float flPathEnd);

    /**
     * Drive the view and advance the path.
     *
     * The first call after Start() records flFrame as the reference. While the fire runs, the
     * path is attached to the view and driven to `flPathStart + (flFrame - reference) * 4`.
     *
     * @param flFrame The current frame.
     * @param flViewFrame The frame the view is driven to.
     * @ghidraAddress 0x004565a8
     */
    void SetFrame(float flFrame, float flViewFrame);

private:
    Rnd::TransAnim *mPath;  // "fx.path".
    Rnd::View *mView;       // "<name>.view".
    int mUnknown08;         // +0x08, never written or read by the recovered routines.
    int mUnknown0c;         // +0x0c, never written or read by the recovered routines.
    TnlEmitter mEmitter;    // "<name>.ps".
    TnlEmitter mAltEmitter; // "<name>a.ps", for an index of -1 only.
    float mPathStartFrame;
    float mPathEndFrame;   // -1 until Start().
    float mReferenceFrame; // 1e9 at construction, -1 from Start() to the next SetFrame().
    int mActive;
    int mIndex;
};
