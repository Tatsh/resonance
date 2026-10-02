#pragma once

#include <iosfwd>
#include <list>

#include "math/transform.h"

class AppTunnel;
class TnlGemEffectKind;
class TnlGemManager;
class TnlGemMeshKind;
namespace Rnd {
struct Particle;
} // namespace Rnd

/**
 * One gem on the tunnel, as TnlGemManager keeps it sorted by mFrame.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the gem kinds it draws. It is 0x24 bytes, the element of the
 * `std::list` at TnlGemManager `+0x18`. AppTunnel builds one on the stack for every gem it places
 * and hands it to TnlGemManager::Add(), which stores a copy.
 *
 * A gem holds no drawing resource until TnlGemManager::Update() places it. Placing a mesh kind adds
 * one instance transform to a TnlGemMeshKind multi-mesh, and placing an effect kind takes one
 * particle from a TnlGemEffectKind particle system. Release() gives the resource back.
 */
class TnlGem {
public:
    /** Progress of the flash a gem shows as the playhead arrives. */
    enum State {
        kStateNone = 0,         /*!< The gem does not flash. */
        kStateFlashPending = 1, /*!< The gem flashes once the playhead is 20 frames short of it. */
        kStateFlashed = 2       /*!< The flash has started. */
    };

    /**
     * Describe a gem that holds no drawing resource and never expires.
     *
     * AppTunnel calls it at `0x004478c4` and `0x00447b40`.
     *
     * @param nKind The kind TnlGemManager::AddMeshKind() or TnlGemManager::AddEffectKind()
     *              returned.
     * @param nTrack The ring of the tunnel the gem sits on.
     * @param nColor The colour index an effect gem's particle takes, as TnlColorIndexFromName()
     *               numbers the player colours.
     * @param bFlash Whether the gem flashes as the playhead arrives.
     * @param flFrame The tunnel frame of the gem.
     * @param flBlend The position across the ring.
     * @param flAppearFrame The tunnel frame from which the gem is drawn.
     * @ghidraAddress 0x00415790
     */
    TnlGem(char nKind,
           char nTrack,
           char nColor,
           bool bFlash,
           float flFrame,
           float flBlend,
           float flAppearFrame);

    /**
     * Place the gem for one frame and report what drawing it costs.
     *
     * A mesh gem takes an instance of the finest TnlGemMeshKind the first time, and then moves its
     * instance to the first level whose mLodDistance, added to flFrame, falls short of mFrame. An
     * effect gem takes a particle the first time, coloured by mColor and sized by the system's low
     * size, and is not moved afterwards. An effect gem whose system has no free particle stays
     * unplaced and costs nothing.
     *
     * @param pManager The manager that owns the gem kinds.
     * @param flFrame The tunnel frame of the playhead.
     * @return The drawing cost of the gem.
     * @ghidraAddress 0x00411e38
     */
    float Place(TnlGemManager *pManager, float flFrame);

    /**
     * Give back the instance or the particle the gem holds.
     *
     * AppTunnel calls it at `0x004483bc`, and TnlGemManager inlines it.
     *
     * @ghidraAddress 0x004157e8
     */
    void Release();

    /**
     * Start a gem flash at the position of the gem's instance or particle.
     *
     * Does nothing for a gem that holds neither. The out-of-line copy has no callers, and
     * TnlGemManager::Update() inlines it.
     *
     * @param pTunnel The tunnel that owns the flash particles.
     * @ghidraAddress 0x00415868
     */
    void Flash(AppTunnel *pTunnel);

    char mKind;         /*!< Below 0x40 a TnlGemMeshKind index, else a TnlGemEffectKind one. */
    char mTrack;        /*!< The ring of the tunnel the gem sits on. */
    char mState;        /*!< The flash progress, one of State. */
    char mColor;        /*!< The colour index of an effect gem's particle. */
    float mFrame;       /*!< The tunnel frame of the gem, the sort key. */
    float mBlend;       /*!< The position across the ring. */
    float mAppearFrame; /*!< The tunnel frame from which the gem is drawn. */
    /*!< The tunnel frame after which the gem is dropped, 1e9 until a later gem at the same frame,
         ring, and position appears. */
    float mExpireFrame;
    TnlGemMeshKind *mMeshKind;     /*!< The level whose multi-mesh holds mInstance, or null. */
    TnlGemEffectKind *mEffectKind; /*!< The kind whose system mParticle belongs to, or null. */
    /*!< The gem's transform in mMeshKind's multi-mesh, set while mMeshKind is. */
    std::list<Transform>::iterator mInstance;
    Rnd::Particle *mParticle; /*!< The gem's particle, set while mEffectKind is. */
};

/**
 * Print a gem as `[kind track frame blend (appear expire)]`.
 *
 * The kind and the track print as characters. The out-of-line copy has no callers.
 *
 * @param stream The stream to print to.
 * @param gem The gem.
 * @return stream.
 * @ghidraAddress 0x00411d30
 */
std::ostream &operator<<(std::ostream &stream, const TnlGem &gem);
