#pragma once

/** Quadwords of the packet buffer a caller may fill before it has to be submitted. */
constexpr int kGifBufferQuadwords = 0x1e0;

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
     * Set the masked part of one GS register, unless it already agrees.
     *
     * The device shadows every GS register in an array of 64-bit words, indexed by register number,
     * and compares the masked incoming value against the shadow before emitting anything. A write
     * that would not change the masked bits is dropped. Callers therefore pass a mask covering only
     * the field they mean to set rather than the whole register.
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
     * @param bEndOfPacket Non-zero to set the tag's end-of-packet bit.
     * @ghidraAddress 0x004a0158
     */
    void CloseGifTag(int bEndOfPacket);

    /**
     * Start a GIFtag at the write pointer.
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
     * The body is not reconstructed. It notifies the GS video memory manager, whose header does not
     * exist yet.
     *
     * @param bRetainOpenTag Non-zero to reopen the current tag in the new buffer half.
     * @param bOnlyWhenFull Non-zero to submit only once the buffer is full.
     * @return Non-zero when a packet was sent.
     * @ghidraAddress 0x0049b478
     */
    int FlushGifPacket(int bRetainOpenTag, int bOnlyWhenFull);

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
    unsigned char mReserved20[0x42c];
    /** Non-zero while geometry is submitted through VU1 rather than the software path. +0x44c */
    int mnUseVu1;
    /** End of the region the end-of-packet path measures against. +0x450 */
    GifQuadword *mpBufferEnd;
};

/**
 * The display device.
 *
 * @ghidraAddress 0x006f2a80
 */
extern GfxDevice g_gfxDevice;
