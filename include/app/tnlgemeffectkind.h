#pragma once

namespace Rnd {
class ParticleSys;
} // namespace Rnd

/**
 * A gem kind that TnlGemManager draws as one particle of a particle system per gem.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from TnlGemManager::AddEffectKind(), which builds one per effect gem
 * ("gem_drum", "gem_bass", and the other instrument gems). The record is 8 bytes, allocated with
 * the untagged scalar allocator.
 */
class TnlGemEffectKind {
public:
    /**
     * Look up "<pszName>.ps" and free every particle it has.
     *
     * The lookup result is used without a null check.
     *
     * @param pszName The base name of the particle system.
     * @ghidraAddress 0x004121d0
     */
    explicit TnlGemEffectKind(const char *pszName);

    Rnd::ParticleSys *mParticleSys; /*!< The system each gem takes one particle from. */
    float mCost;                    /*!< The drawing cost of one gem, always 2. */
};
