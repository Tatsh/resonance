#pragma once

#include "math/color.h"
#include "math/transform.h"
#include "os/hxstr.h"

/**
 * Map a player colour name to the colour the tunnel draws that player in.
 *
 * "green" gives green, "red" red, "yellow" yellow, "purple" `(0.65, 0, 1)`, and "null" white. Every
 * other name gives cyan. The alpha is always 1. The title is inferred from the five literals the
 * routine compares against.
 *
 * @param name The colour name, taken by value.
 * @return The colour.
 * @ghidraAddress 0x00437e60
 */
Color TnlColorFromName(HxStr name);

/**
 * Map a player colour name to a darker form of TnlColorFromName().
 *
 * The primary components are 0.7 rather than 1, "purple" gives `(0.5, 0, 0.7)`, and "null" gives
 * grey 0.7. Every other name gives cyan, and the alpha is always 1. The program lists no caller.
 *
 * @param name The colour name, taken by value.
 * @return The colour.
 * @ghidraAddress 0x00437fa8
 */
Color TnlDimColorFromName(HxStr name);

/**
 * Map a player colour name to the lane floor colour, scaled by g_flTunnelBrightness.
 *
 * The primary components are 0.5, "purple" gives `(0.325, 0, 0.5)`, and "null" gives
 * `(0, 0, 0.25)`, each scaled through ScaleColor(). Every other name gives unscaled white.
 *
 * @param name The colour name, taken by value.
 * @return The colour.
 * @ghidraAddress 0x00438138
 */
Color TnlLaneColorFromName(HxStr name);

/**
 * Map a player colour name to its index.
 *
 * "null" gives 0, "green" 1, "red" 2, "yellow" 3, and "purple" 4. Every other name gives 0. The
 * program lists no caller.
 *
 * @param name The colour name, taken by value.
 * @return The index.
 * @ghidraAddress 0x00454770
 */
int TnlColorIndexFromName(HxStr name);

/**
 * Set the padding word of every row of a transform to 1.
 *
 * Matches the Vector3 constructor the image runs on each row of a stack transform before handing
 * it to Rnd::Tunnel. No other word of the transform is written.
 *
 * @param xfm The transform to pad.
 */
inline void PadTransformRows(Transform &xfm) {
    xfm.mBasisX.w = 1.0f;
    xfm.mBasisY.w = 1.0f;
    xfm.mBasisZ.w = 1.0f;
    xfm.mTranslation.w = 1.0f;
}

/**
 * Result of configuration code 0x3a1 when AppTunnel was constructed.
 *
 * AppTunnel's constructor writes it at `0x00442344`. A non-zero value stops TnlActivator::Update()
 * from blinking the activator and catcher materials.
 *
 * @ghidraAddress 0x006e42a4
 */
extern int g_nAppTunnelDisplayMode;

/**
 * The brightness the lane floor colours are scaled by.
 *
 * AppTunnel's constructor sets it to 2 for a game of two to four players, which use the
 * split-screen tunnel, and to 1 otherwise, when it also adds "lat light1" to "tunnel.env".
 *
 * @ghidraAddress 0x006e42b0
 */
extern float g_flTunnelBrightness;
