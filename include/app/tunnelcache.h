#pragma once

namespace Rnd {
class Tunnel;
} // namespace Rnd

// The image has no embedded path for this translation unit. The title comes from the one literal
// it formats, "tunnel", and from the object it records. The unit also defines the out-of-line
// copies of Rnd::Animatable::SetRate(), SetOffset(), and SetLoopRange(), and of
// Rnd::View::AddView() and RemoveView().

/**
 * Resolve the loaded object "tunnel" and record it in g_pTunnel.
 *
 * An object of that name that is not an Rnd::Tunnel records null. AppTunnel's constructor is the
 * caller.
 *
 * @ghidraAddress 0x0040d1e8
 */
void CacheTunnelObjectByName();

/**
 * Report the tunnel CacheTunnelObjectByName() last recorded.
 *
 * @return g_pTunnel.
 * @ghidraAddress 0x0040f6c0
 */
Rnd::Tunnel *GetCachedTunnelObject();

/**
 * Tunnel CacheTunnelObjectByName() last recorded, or null.
 *
 * @ghidraAddress 0x006de808
 */
extern Rnd::Tunnel *g_pTunnel;
