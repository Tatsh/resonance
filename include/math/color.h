#pragma once

/**
 * Linear colour with four float components.
 *
 * The class is not polymorphic and emits no RTTI descriptor, so the name is inferred. The text
 * dump of a material colour and of a mesh vertex colour writes four components under the labels
 * "(r:", " g:", " b:", and " a:", and the renderer copies the four words as one quadword.
 */
struct Color {
    float r;
    float g;
    float b;
    float a;
};

/**
 * Clamp all four components of a colour to the unit range.
 *
 * The vector unit raises the floor with a maximum against a zero broadcast and lowers the ceiling
 * with a minimum against a 1.0f broadcast, then stores the result as one quadword. All four
 * components pass through both stages, so the alpha word is clamped along with the three colour
 * words. The one caller is `Rnd::PsMesh::RefreshAfterLoad()`.
 *
 * The title is inferred. No literal in the image identifies the routine.
 *
 * @param source The colour to clamp.
 * @param result Receives the clamped colour. It may alias the source.
 * @ghidraAddress 0x00607268
 */
void ClampColorToUnitRange(const Color &source, Color &result);

/**
 * Add two colours component by component.
 *
 * The result is a parameter rather than a return value, and that is measured rather than assumed.
 * A by-value operator in this ABI receives a hidden result pointer as its first argument and hands
 * it back in the return register, whereas this routine writes through its third argument and never
 * writes the return register at all. So it is a named routine of three parameters and not
 * `operator+`.
 *
 * Both call paths operate on colours. The particle system interpolates a colour with it, and the
 * mesh weld path sums vertex colours, reaching each one at the colour offset of a vertex.
 *
 * The title is inferred. No literal in the image identifies either routine, and nothing
 * distinguishes an addition named this way from one named any other way.
 *
 * @param left The first colour.
 * @param right The second colour.
 * @param result Receives the sum. It may alias either operand, and the mesh weld path passes the
 *               same address as both the first operand and the result.
 * @ghidraAddress 0x004924a8
 */
void AddColor(const Color &left, const Color &right, Color &result);

/**
 * Subtract one colour from another component by component.
 *
 * The body is the addition above with a subtracting operation in place of the adding one, and the
 * same reasoning fixes its shape and leaves its title inferred.
 *
 * @param left The colour subtracted from.
 * @param right The colour to subtract.
 * @param result Receives the difference. It may alias either operand.
 * @ghidraAddress 0x0052b258
 */
void SubColor(const Color &left, const Color &right, Color &result);
