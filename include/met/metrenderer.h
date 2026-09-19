#pragma once

#include "rnd/view.h"

// MetScreen stores its renderer and the renderer stores its screens, so one of the two
// declarations has to be incomplete. MemcardOp declares MemcardCBHandler the same way.
class MetScreen;

/**
 * Front-end renderer that owns the screen stack and drives every MetScreen.
 *
 * `11MetRenderer` in the RTTI descriptor at `0x008eef98`, with three public non-virtual bases at
 * fixed offsets, MsgSource at `+0x00`, RendererBase at `+0x14`, and FadeUser at `+0x5c`. Its
 * GetTypeInfo is at `0x00370f98`.
 *
 * This declaration is deliberately partial. The class belongs to the renderer half of the
 * front end. Only the part of it that MetScreen uses is recovered here, the two fields below and
 * the four member functions that MetScreen calls with the renderer as their first argument. Every
 * one of the four titles is inferred from its body, because a C++ method name survives nowhere in
 * the image. The bases are documented rather than written, because RendererBase is not
 * reconstructed yet.
 *
 * MetScreen stores its renderer at `+0x10` and registers itself on it as a message sink through
 * MsgSource::AddSink() during construction, which is how the pointer is known to address the
 * MsgSource subobject at offset 0.
 *
 * The screen stack is the `std::vector<MetScreen *>` at `+0x84` through `+0x8c`, which AddScreen()
 * appends to and RemoveScreen() erases from. Both write 1 to the flag at `+0x98` once they have
 * changed the stack. Neither the vector nor the flag is declared, because the span they sit in is
 * not otherwise recovered.
 */
class MetRenderer {
public:
    /**
     * Record one screen as the active panel.
     *
     * Stores pScreen at `+0x7c` and then, when the object at `+0x94` is present, hands that object
     * to the routine at `0x002e7298`. The title is inferred from the field MetScreen slot 6 pairs
     * the call with.
     *
     * @param pScreen The screen to record.
     * @ghidraAddress 0x003714c8
     */
    void SetActivePanel(MetScreen *pScreen);

    /**
     * Attach the three animatable, drawable, and transformable subobjects of one view to the scene.
     *
     * Each of the three is appended only when the scene does not already store it, which the three
     * membership tests at `0x00370ab8`, `0x00370b08`, and `0x00370b58` decide. A null view is
     * passed through to all three as null rather than rejected. The title is inferred.
     *
     * @param pView The view to attach.
     * @ghidraAddress 0x003717b0
     */
    void AddScreenView(Rnd::View *pView);

    /**
     * Append one screen to the screen stack.
     *
     * A screen already on the stack is not appended a second time. The title is inferred from the
     * vector the body appends to.
     *
     * @param pScreen The screen to append.
     * @ghidraAddress 0x003719e0
     */
    void AddScreen(MetScreen *pScreen);

    /**
     * Erase one screen from the screen stack and detach its view from the scene.
     *
     * A screen absent from the stack does nothing. The title is inferred.
     *
     * @param pScreen The screen to erase.
     * @ghidraAddress 0x00371a78
     */
    void RemoveScreen(MetScreen *pScreen);

    /**
     * Span from the start of the object to mUnknown68, which is not recovered.
     *
     * The three base subobjects occupy `+0x00` through `+0x5f`, and what follows them up to
     * mUnknown68 is undetermined.
     *
     * +0x00
     */
    unsigned char mReserved00[0x68];

    /**
     * Current front-end time in seconds.
     *
     * MetScreen reads this field and passes it straight to its own enter and exit animation
     * virtuals at vtable slots 31 and 34, both of which take a float.
     *
     * +0x68
     */
    float mUnknown68;

    /**
     * Span between mUnknown68 and mUnknown80, which is not recovered.
     *
     * +0x6c
     */
    unsigned char mReserved6c[0x14];

    /**
     * Flag that MetScreen sets when it activates a named sub-screen and clears when it activates
     * none.
     *
     * Written directly by MetScreen vtable slot 6 at `0x0038b828`, which stores 0 for the empty
     * name and 1 once the named screen reports that it has finished loading. No accessor for the
     * field appears in the image, so it is public.
     *
     * +0x80
     */
    int mUnknown80;
};
