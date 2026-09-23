#pragma once

/** Quadwords of the packet buffer a caller may fill before it has to be submitted. */
constexpr int kGifBufferQuadwords = 0x1e0;

/** GS registers the device shadows, the whole register address space. */
constexpr int kGsRegisterCount = 128;

/**
 * One quadword of a packet stream.
 *
 * Every pointer into the buffer is typed as this rather than as bytes, because the hardware moves
 * the stream a quadword at a time and every offset in the routines below is a quadword count.
 */
struct GifQuadword {
    unsigned long long mLo;
    unsigned long long mHi;
};

/**
 * Owner of the display, the GIF packet buffer, and the frame timing counters.
 *
 * The class is not polymorphic and has no RTTI. Its name is inferred from its methods. They build
 * GIF packets, set GS registers, and flip the framebuffer.
 *
 * Packets are assembled in PlayStation 2 scratchpad memory and double buffered between
 * `0x70000000` and `0x70002000`, the two halves of the 16 KiB region. Submitting a packet kicks a
 * DMA channel at the half just filled and moves the write pointer to the other half. The next
 * packet is therefore built while the previous one is in flight. The address given to the DMA
 * controller is derived from the buffer base by moving bit 30 up to bit 31, the scratchpad flag.
 */
class GfxDevice {
public:
    /**
     * Bring up the display and the drawing subsystems.
     *
     * Registers the device profile timers ("setup", "vram", "billboard", "vert", "prim", "sync"),
     * records the display geometry, and initialises the material, texture, and mesh backends.
     *
     * @param nWidth The display width in pixels.
     * @param nHeight The display height in pixels.
     * @param nBitDepth The framebuffer depth in bits.
     * @ghidraAddress 0x0049ae20
     */
    void Init(int nWidth, int nHeight, int nBitDepth);

    /**
     * Start a frame.
     *
     * @ghidraAddress 0x0049b930
     */
    void BeginFrame();

    /**
     * Finish a frame and show it.
     *
     * @param nSwapBuffers Non-zero to flip the framebuffer.
     * @ghidraAddress 0x0049bac8
     */
    void PresentFrame(int nSwapBuffers);

    /**
     * Program the GS display registers for the recorded geometry.
     *
     * @ghidraAddress 0x0049b138
     */
    void InitDisplayMode();

    /**
     * Words of video memory the display and depth buffers occupy.
     *
     * VramTable::Init() divides the result by the words in a block to find where the palette
     * region begins, which puts every cached texture above the buffers. The body is not
     * reconstructed. It reads three members of the recorded geometry and a page count out of the
     * double buffer descriptor mpDisplayBuffers addresses, whose layout is unrecovered.
     *
     * @return Words in use, at four bytes each.
     * @ghidraAddress 0x0049fec0
     */
    int GetReservedVramWords() const;

    /**
     * Set the masked part of one GS register, unless it already agrees.
     *
     * The device shadows every GS register in an array of 64-bit words, indexed by register number,
     * and compares the masked incoming value against the shadow before emitting anything. A write
     * that would not change the masked bits is dropped. Callers therefore pass a mask covering only
     * the field they mean to set rather than the whole register.
     *
     * A write that does change the register opens an A+D GIFtag from mAdTag unless the open tag is
     * already one, and then submits the buffer if it has filled. Setting PRIM to a line strip, a
     * triangle strip, or a triangle fan leaves the shadow's primitive type at 7, a value no caller
     * sets. The next PRIM write therefore always reaches the GS and starts a new strip.
     *
     * @param nReg The GS register number.
     * @param qwValue The value to set, of which only the masked bits are used.
     * @param qwMask The bits of the register this call owns.
     * @ghidraAddress 0x0049ffd0
     */
    void SetGsReg(int nReg, unsigned long long qwValue, unsigned long long qwMask);

    /**
     * Divert writes into a region carved off the end of the packet buffer.
     *
     * The region begins where the requested number of quadwords ends the buffer. The current write
     * pointer is preserved for SwapGifWrite() to return to. A second reservation while one is open
     * does nothing, and the whole routine does nothing while the software path is selected.
     *
     * A caller reserves space, programs its registers into the region, swaps back to the main
     * stream to append the bulk data, and finally splices the region in with
     * FlushReservedGif(). That order exists because the register writes are only known to be
     * needed once the bulk data has been measured.
     *
     * @param nQuadwords The space to reserve.
     * @ghidraAddress 0x0049fd98
     */
    void ReserveGifSpace(int nQuadwords);

    /**
     * Exchange the write pointer with the one the reservation preserved.
     *
     * Calling it twice returns to where it started. That is how a caller alternates between the
     * reserved region and the main stream. It does nothing unless a reservation is open.
     *
     * @ghidraAddress 0x0049fe08
     */
    void SwapGifWrite();

    /**
     * Copy the reserved region into the main stream and close the reservation.
     *
     * The quadword count comes from how far the region's own write pointer advanced. Nothing is
     * copied when the region is empty, and the reservation is closed either way.
     *
     * @ghidraAddress 0x0049fe38
     */
    void FlushReservedGif();

    /**
     * Fill in the loop count of the open GIFtag, and optionally end the packet.
     *
     * The count is the quadwords written after the tag divided by the registers each loop consumes.
     * That divisor is the tag's own NREG shifted right by its FLG. The same expression is correct
     * for all three formats. A packed loop spends one quadword per register, a register list spends
     * one per two, and an image loop spends one per four.
     *
     * Ending the packet on the VU1 path also fills in the count of the open VIF DIRECT code.
     *
     * @param bEndOfPacket Non-zero to set the tag's end-of-packet bit.
     * @ghidraAddress 0x004a0158
     */
    void CloseGifTag(int bEndOfPacket);

    /**
     * Start a GIFtag at the write pointer.
     *
     * The open tag is closed first without the end-of-packet bit. On the VU1 path a VIF DIRECT
     * code goes ahead of the tag unless one is already open.
     *
     * @param pTag The tag to write, whose loop count CloseGifTag() fills in later.
     * @ghidraAddress 0x0049b5a8
     */
    void WriteGifTag(const GifQuadword *pTag);

    /**
     * Send the assembled packet and move to the other scratchpad half.
     *
     * Closes the open tag with the end-of-packet bit, then kicks VIF1 while the VU1 path is
     * selected and the GIF otherwise. An empty buffer is not sent. Retaining the open tag reopens
     * it in the new half, letting a caller submit in the middle of a primitive.
     *
     * Every submission advances the video memory cache to its next lock generation and then runs
     * the long operation poll callback through RunLongOperationPollProc().
     *
     * @param bRetainOpenTag Non-zero to reopen the current tag in the new buffer half.
     * @param bOnlyWhenFull Non-zero to submit only once the buffer is full.
     * @return Non-zero when a packet was sent.
     * @ghidraAddress 0x0049b478
     */
    int FlushGifPacket(int bRetainOpenTag, int bOnlyWhenFull);

    /**
     * Route later packets through VIF1 and VU1 rather than straight to the GIF.
     *
     * Does nothing when the VU1 path is already selected. Otherwise it submits whatever the
     * buffer has before switching, because a packet built for one path cannot be sent down the
     * other.
     *
     * @ghidraAddress 0x004a0348
     */
    void EnterVu1Path();

    /**
     * Return to sending packets straight to the GIF.
     *
     * Does nothing unless the VU1 path is selected. Otherwise it ends the open tag, submits the
     * buffer, and waits for the GS paths to drain before switching.
     *
     * @ghidraAddress 0x0049b838
     */
    void LeaveVu1Path();

    /**
     * Point FRAME_1 and XYOFFSET_1 back at the display buffer being drawn.
     *
     * Rnd::PsCam::DrawSelf() calls it when the previous camera drew into a texture. Both values
     * come from the draw environment of the current half of the double buffer descriptor that
     * mpDisplayBuffers addresses, and each goes through the body of SetGsReg() open-coded. The body
     * is not reconstructed, because the descriptor's layout is unrecovered.
     *
     * @ghidraAddress 0x0049b6b8
     */
    void RestoreFrameBufferTarget();

    // The layout is recovered only where the packet routines read it. A reserved run records a span
    // that has not been recovered and is not a field.

    /** Where the next quadword is written. */
    GifQuadword *mpWrite;
    /** Scratchpad half currently being filled. */
    GifQuadword *mpBuffer;
    /** Write pointer preserved across a reservation. */
    GifQuadword *mpSavedWrite;
    /** Start of the reserved region, or null when none is open. */
    GifQuadword *mpReservedRegion;
    unsigned char mReserved10[0x0c];
    /** GIFtag whose loop count is still to be filled in, or null when none is open. */
    GifQuadword *mpOpenTag;
    /** Display width in pixels, as Init() recorded it. Rnd::PsCam::ScreenToPixels() reads it. */
    int mnDisplayWidth;
    /** Display height in pixels. Read alongside the width by the same routine. */
    int mnDisplayHeight;
    /** Framebuffer bytes per pixel, the Init() bit depth rounded up to whole bytes. +0x28 */
    int mnPixelBytes;
    /**
     * Depth buffer bytes per sample, which InitDisplayMode() derives from mnPixelBytes.
     *
     * Rnd::PsCam::DrawSelf() reads it to find the largest depth value. +0x2c
     */
    int mnDepthBytes;
    /** A+D GIFtag SetGsReg() opens when the open tag is not already an A+D tag. +0x30 */
    GifQuadword mAdTag;
    /** Last value written to each GS register, indexed by register number. +0x40 */
    unsigned long long mGsRegs[kGsRegisterCount];
    unsigned char mReserved440[0x04];
    /** Half of the double buffer being drawn, which RestoreFrameBufferTarget() selects by. +0x444
     */
    int mnDrawBuffer;
    /**
     * Double buffer descriptor InitDisplayMode() builds through the SDK, at its uncached address.
     *
     * The descriptor is the SDK's rather than the game's, and its layout is unrecovered.
     * GetReservedVramWords() reads a page count from it. +0x448
     */
    void *mpDisplayBuffers;
    /** Non-zero while geometry is submitted through VU1 rather than the software path. +0x44c */
    int mnUseVu1;
    /**
     * First quadword after the VIF DIRECT code that WriteGifTag() opened, or null.
     *
     * On the VU1 path every run of GIF data travels inside a DIRECT code, whose immediate is the
     * quadword count that follows it. WriteGifTag() writes the code with a count of zero, and
     * CloseGifTag() with the end-of-packet bit fills the count in from this pointer. +0x450
     */
    GifQuadword *mpOpenVifDirect;
};

/**
 * The display device.
 *
 * @ghidraAddress 0x006f2a80
 */
extern GfxDevice g_gfxDevice;
