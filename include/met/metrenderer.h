#pragma once

/**
 * Front-end renderer that owns the screen stack and drives every MetScreen.
 *
 * `11MetRenderer` in the RTTI descriptor at `0x008eef98`, with three public non-virtual bases at
 * fixed offsets, MsgSource at `+0x00`, RendererBase at `+0x14`, and FadeUser at `+0x5c`. Its
 * GetTypeInfo is at `0x00370f98`.
 *
 * This declaration is deliberately partial. The class belongs to the renderer half of the
 * front end. Only the part of it that MetScreen uses is recovered here, the two fields below and
 * four member functions that MetScreen calls with the renderer as their first argument, at
 * `0x003714c8`, `0x003717b0`, `0x003719e0`, and `0x00371a78`. None of the four has a recovered
 * name, so all four are recorded rather than declared. The bases are documented rather than
 * written, because RendererBase is not reconstructed yet.
 *
 * MetScreen stores its renderer at `+0x10` and registers itself on it as a message sink through
 * MsgSource::AddSink() during construction, which is how the pointer is known to address the
 * MsgSource subobject at offset 0.
 */
class MetRenderer {
public:
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
