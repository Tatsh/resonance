#pragma once

namespace Rnd {
class Drawable;
class MultiMesh;
} // namespace Rnd

/**
 * One level of detail of a gem kind that TnlGemManager draws as instances of a multi-mesh.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from TnlGemManager::AddMeshKind(), which builds one per "<name><n>.mm"
 * multi-mesh it finds and links them through mNext from the finest level to the coarsest. The
 * record is 0x10 bytes, allocated with the untagged scalar allocator.
 */
class TnlGemMeshKind {
public:
    /**
     * Take a multi-mesh as one level of detail and clear its instances.
     *
     * The level applies while a gem lies further ahead than the instanced mesh's
     * Rnd::Mesh::mMinScreen plus flLodOffset. The cost is the triangle count of the instanced
     * mesh times flCostScale. The out-of-line copy has no callers, and TnlGemManager::AddMeshKind()
     * inlines it.
     *
     * @param pMesh The multi-mesh.
     * @param flLodOffset The frames added to the instanced mesh's level of detail threshold.
     * @param flCostScale The cost of one triangle.
     * @ghidraAddress 0x004158c0
     */
    TnlGemMeshKind(Rnd::MultiMesh *pMesh, float flLodOffset, float flCostScale);

    /**
     * Add the multi-mesh of this level and of every coarser level to a drawable.
     *
     * Does nothing when pParent is null. Each multi-mesh is added at the end of the draw list.
     *
     * @param pParent The drawable to add to, or null.
     * @ghidraAddress 0x00415980
     */
    void AddDrawTo(Rnd::Drawable *pParent);

    /**
     * Show or hide the multi-mesh of this level and of every coarser level.
     *
     * @param nShowing Non-zero to show.
     * @ghidraAddress 0x004159d8
     */
    void SetShowing(int nShowing);

    /**
     * Report the drawing cost one gem at this level adds.
     *
     * @return mCost while the multi-mesh is showing, zero otherwise.
     * @ghidraAddress 0x00415a30
     */
    float GetCost();

    Rnd::MultiMesh *mMesh; /*!< The multi-mesh one instance per gem is added to. */
    /*!< Frames ahead of the playhead a gem has to lie for this level to draw it. */
    float mLodDistance;
    TnlGemMeshKind *mNext; /*!< The next coarser level, or null. */
    float mCost;           /*!< The drawing cost of one gem at this level. */
};
