#pragma once

#include <vector>

#include "msg/barstatusmsg.h"

namespace Rnd {
class Font;
class Mat;
class Mesh;
class Text;
} // namespace Rnd

/**
 * Effect lamps of one player's track display on the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the `fx` objects it resolves. HudTrack embeds one at `+0x58`.
 *
 * The constructor builds one lamp per item kind that configuration code 0x389 lists, in list order.
 * Each lamp is a mesh whose material shows the lamp lit or unlit, and a text naming the kind whose
 * font shows it selected or not.
 */
class HudEffects {
public:
    /**
     * One lamp, a plain twelve-byte record.
     *
     * The name is inferred.
     */
    struct Lamp {
        Rnd::Mesh *mMesh; /*!< `<layout> fx<n><i>.mesh`. +0x00 */
        Rnd::Text *mText; /*!< `<layout> fx<n><i>.txt`, showing HudPowerupName(). +0x04 */
        int mKind;        /*!< The HudItemKind the lamp stands for. +0x08 */
    };

    /**
     * Resolve the shared materials and fonts and build one lamp per listed kind, all unlit.
     *
     * The wires, `<layout> fxwires<n>.mesh`, show only in kPlayModeJam.
     *
     * @param nIndex The track display number that fills `<n>`.
     * @ghidraAddress 0x00417f50
     */
    HudEffects(int nIndex);

    /**
     * Light every lamp whose kind's bit is set in a mask, and darken the rest.
     *
     * The kHudItemGuides lamp does not change. Overlay::OnBarChanged() passes the effect mask a
     * BarStatusMsg reports. The title is inferred.
     *
     * @param effects One bit per item kind, bit n for kind n.
     * @ghidraAddress 0x00418760
     */
    void SetMask(BarStatusMsg::Effects effects);

    /**
     * Show one kind's name in the selected font and every other name in the plain font.
     *
     * The out-of-line copy has no caller. The title is inferred.
     *
     * @param nKind The HudItemKind to select.
     * @ghidraAddress 0x00429e98
     */
    void Select(int nKind);

    /**
     * Light or darken the lamps of one kind.
     *
     * The title is inferred.
     *
     * @param nKind The HudItemKind.
     * @param nLit Non-zero to light.
     * @ghidraAddress 0x00429f28
     */
    void SetLit(int nKind, int nLit);

private:
    Rnd::Mesh *mWires;        // `<layout> fxwires<n>.mesh`
    Rnd::Mat *mLitMat;        // `HUD fx_on.mat`
    Rnd::Mat *mUnlitMat;      // `HUD fx_off.mat`
    Rnd::Font *mSelectedFont; // `HUD fx_on.font`
    Rnd::Font *mPlainFont;    // `HUD fx_off.font`
    std::vector<Lamp> mLamps;
};
