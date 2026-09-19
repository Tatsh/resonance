#pragma once

#include "rnd/tex.h"

/**
 * @file
 *
 * Lookup of the numbered persona burn textures.
 *
 * The one routine here has no receiver argument and vends a Rnd::Tex it does not manage, so it is
 * reconstructed as a free function rather than as a member. Eleven call sites across the metagame
 * screens use it, and no class in the image declares it.
 */

/**
 * Resolve one numbered persona burn texture out of the render manager.
 *
 * The name is formatted as `persona_texburn_texture_%d.tex` from one past the index, so index zero
 * resolves `persona_texburn_texture_1.tex`. A name the manager does not recognise produces a null
 * result, as does an object that is not a Rnd::Tex. Nothing in the image attests the routine name,
 * so it follows the required style; the Ghidra program records it as FindPersonaBurnTexture.
 *
 * @param index Zero-based texture index.
 * @return The texture, or null when the manager has no such object.
 * @ghidraAddress 0x001712c0
 */
Rnd::Tex *findPersonaBurnTexture(int index);
