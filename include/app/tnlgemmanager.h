#pragma once

#include <list>
#include <vector>

#include "app/tnlgem.h"

class AppTunnel;
class HxStr;
class TnlGemEffectKind;
class TnlGemMeshKind;
namespace Rnd {
class Drawable;
} // namespace Rnd

/**
 * Keeper of every gem on the tunnel, which it draws within a per-frame cost budget.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from what it manages. AppTunnel's constructor allocates it (0x2c bytes,
 * untagged) at `0x00443540`, stores it at AppTunnel `+0x84`, and registers every gem kind through
 * AddMeshKind() and AddEffectKind(). AppTunnel's destructor deletes it at `0x00445790`.
 *
 * A gem kind below kEffectKindBase indexes mMeshKinds, and a kind at or above it indexes
 * mEffectKinds after kEffectKindBase is subtracted.
 */
class TnlGemManager {
public:
    /** The first kind AddEffectKind() returns, and the bit that marks an effect kind. */
    enum { kEffectKindBase = 0x40 };

    /**
     * Start with no gem kind and no gem.
     *
     * @param pTunnel The tunnel that owns the manager.
     * @param flCostBudget The drawing cost Update() places gems up to in one frame.
     * @ghidraAddress 0x004122c0
     */
    TnlGemManager(AppTunnel *pTunnel, float flCostBudget);

    /**
     * Delete every gem kind.
     *
     * The gems are dropped without Release().
     *
     * @ghidraAddress 0x00412490
     */
    ~TnlGemManager();

    /**
     * Register every level of detail of one mesh gem kind.
     *
     * Looks up "<pszName>0.mm", "<pszName>1.mm", and so on until a name is missing or not a
     * multi-mesh, and links the levels found from the first to the last through
     * TnlGemMeshKind::mNext.
     *
     * @param pszName The base name, such as "gem_hex_g".
     * @param flLodOffset The frames added to every level's threshold.
     * @param flCostScale The cost of one triangle.
     * @return The kind of the first level, the number of levels registered before the call.
     * @ghidraAddress 0x00412628
     */
    char AddMeshKind(const char *pszName, float flLodOffset, float flCostScale);

    /**
     * Register one effect gem kind from "<pszName>.ps".
     *
     * @param pszName The base name, such as "gem_drum".
     * @return The kind, kEffectKindBase plus the number of effect kinds registered before the call.
     * @ghidraAddress 0x00412878
     */
    char AddEffectKind(const char *pszName);

    /**
     * Insert a copy of a gem after every gem at or before its frame.
     *
     * Every gem already at the same frame, ring, and position expires at the new gem's
     * mAppearFrame.
     *
     * @param gem The gem.
     * @ghidraAddress 0x00412968
     */
    void Add(const TnlGem &gem);

    /**
     * Release and remove every gem of one ring from flStart up to flEnd.
     *
     * @param nTrack The ring.
     * @param flStart The first frame removed.
     * @param flEnd The frame the removal stops before.
     * @ghidraAddress 0x00412b18
     */
    void RemoveRange(char nTrack, float flStart, float flEnd);

    /**
     * Release and remove every gem at one frame, ring, and position.
     *
     * @param nTrack The ring.
     * @param flFrame The frame.
     * @param flBlend The position across the ring.
     * @ghidraAddress 0x00412c50
     */
    void Remove(char nTrack, float flFrame, float flBlend);

    /**
     * Drop the gems the playhead has passed and place the rest within the budget.
     *
     * A gem more than mLateWindow frames behind the playhead, or past its mExpireFrame, is released
     * and removed. A pending flash starts 20 frames before its gem. Visible gems are placed in
     * frame order until mCostBudget or mMaxPlaced is used up, and the rest release their drawing
     * resources. mMaxPlaced then shrinks to the number placed, or grows by one when the limit was
     * reached. AppTunnel::SetFrame() calls it at `0x00446a08`.
     *
     * @param flFrame The tunnel frame of the playhead.
     * @ghidraAddress 0x00412db0
     */
    void Update(float flFrame);

    /**
     * Report the finest level of one mesh gem kind.
     *
     * @param nKind The kind.
     * @return The level.
     * @ghidraAddress 0x00415a50
     */
    TnlGemMeshKind *GetMeshKind(char nKind);

    /**
     * Report one effect gem kind.
     *
     * @param nKind The kind, kEffectKindBase or above.
     * @return The kind record.
     * @ghidraAddress 0x00415a68
     */
    TnlGemEffectKind *GetEffectKind(char nKind);

    /**
     * Add every level of one mesh gem kind to a drawable.
     *
     * Does nothing for an effect kind. AppTunnel::ShowTrackGhost() calls it at `0x00457450`.
     *
     * @param nKind The kind.
     * @param pParent The drawable to add to, or null.
     * @ghidraAddress 0x00415a88
     */
    void AddKindDraws(char nKind, Rnd::Drawable *pParent);

    /**
     * Show or hide every level of one mesh gem kind.
     *
     * Does nothing for an effect kind. AppTunnel calls it at `0x00443880`, `0x00446514`, and
     * `0x00457468`.
     *
     * @param nKind The kind.
     * @param nShowing Non-zero to show.
     * @ghidraAddress 0x00415af8
     */
    void SetKindShowing(char nKind, int nShowing);

private:
    // AppTunnel's PlaybackToggleMsg handler walks mGems to reassign each gem's kind.
    friend class AppTunnel;

    // Find the first gem whose mFrame is at or after flFrame. 0x00415e78.
    static std::list<TnlGem>::iterator FindFirstAt(std::list<TnlGem> &gems, float flFrame);

    std::vector<TnlGemMeshKind *> mMeshKinds;     // Every level, each kind from its own index.
    std::vector<TnlGemEffectKind *> mEffectKinds; // Every effect kind.
    std::list<TnlGem> mGems;                      // Sorted by TnlGem::mFrame.
    AppTunnel *mTunnel;                           // Owner of the flash particles.
    float mCostBudget;                            // Cost Update() places gems up to per frame.
    float mLateWindow;                            // Frames behind the playhead a gem is kept.
    int mMaxPlaced;                               // Gems Update() places at most per frame.
};

/**
 * Create the next name of the form "<tnlmesh0000>".
 *
 * The number is the incremented g_nTnlMeshNameCounter. The out-of-line copy has no callers, and no
 * inlined copy is known. The routine takes no object and belongs to no identified class.
 *
 * @return The name.
 * @ghidraAddress 0x00415668
 */
HxStr NextTnlMeshName();

/**
 * The number of the last NextTnlMeshName() name.
 *
 * @ghidraAddress 0x006df360
 */
extern int g_nTnlMeshNameCounter;

/**
 * The tunnel frame TnlGemManager::Update() last ran for.
 *
 * Update() writes it and nothing reads it.
 *
 * @ghidraAddress 0x006df364
 */
extern float g_flTnlGemLastFrame;
