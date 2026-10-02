#pragma once

namespace Rnd {
class View;
} // namespace Rnd

/**
 * One view group of the head-up display, `tc_hi_group.view`.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the one object it resolves. The head-up display panel embeds one at
 * `+0x110`.
 */
class HudTcGroup {
public:
    /**
     * Resolve the group and hide it.
     *
     * @ghidraAddress 0x00417a60
     */
    HudTcGroup();

    /**
     * Show or hide the group.
     *
     * The constructor is the one caller. A byte-identical body at `0x00455390` belongs to another
     * translation unit. The title is inferred.
     *
     * @param nShowing Non-zero to show.
     * @ghidraAddress 0x00429e08
     */
    void SetShowing(int nShowing);

private:
    Rnd::View *mView; // `tc_hi_group.view`
};
