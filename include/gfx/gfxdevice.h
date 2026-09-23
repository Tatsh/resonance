#pragma once

#include "math/color.h"

class GsDoubleBuffer;

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
     * Rectangle of four floats.
     *
     * SetupGsDrawContext() measures one in fractions of the display, and the debug overlay in GS
     * primitive pixels. The name and the fields are inferred. The type is four floats aligned to a
     * word rather than a Color. Renderer::OnUnknownSlot8() copies one into mFeedbackRect with
     * unaligned doubleword loads and stores (ldl, sdl), whereas SetClearColor() copies its Color
     * with quadword loads and stores (lq, sq). The compiler emits a quadword access only for a
     * 16-byte aligned type.
     */
    struct Rect {
        float x; /*!< Left edge. */
        float y; /*!< Top edge. */
        float w; /*!< Width. */
        float h; /*!< Height. */
    };

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
     * region begins. The depth buffer page, at 2048 words a page, locates the end of both frame
     * buffers, and the display geometry at mnDepthBytes a pixel adds the depth buffer itself.
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
     * come from the draw environment of the half mnDrawBuffer selects, and the binary open-codes
     * the body of SetGsReg() for each.
     *
     * @ghidraAddress 0x0049b6b8
     */
    void RestoreFrameBufferTarget();

    /**
     * Record the colour both halves of the double buffer clear to.
     *
     * Stores the colour in mClearColor and writes it into the RGBAQ pair of each half's clear,
     * red, green, and blue scaled by 255, alpha by 128, and Q at 1.0. The data cache is then
     * written back, because the double buffer is read through its uncached alias.
     *
     * @param color The clear colour, each component from 0 to 1.
     * @ghidraAddress 0x0049b368
     */
    void SetClearColor(const Color &color);

    /**
     * Draw the render statistics as debug text.
     *
     * One line each for the frame rate and for every RenderStats counter, then the kilobytes of
     * texture the VRAM table uploaded in the last frame.
     *
     * @ghidraAddress 0x0049bec8
     */
    void DrawRenderStatsOverlay();

    /**
     * Draw one bar per profiler timer, scaled against a full-scale time.
     *
     * @param nFullScaleMs Milliseconds a bar spanning 95 per cent of the display width stands for.
     * @ghidraAddress 0x0049c778
     */
    void DrawSubsystemTimingGraph(int nFullScaleMs);

    /**
     * Blend the displayed frame back over the frame being drawn.
     *
     * Disables the depth test and depth writes, sets ALPHA_1 to blend by mFeedbackAlpha, and
     * binds the frame buffer of the half not being drawn as a 1024 by 1024 texture. It then draws
     * one sprite over mFeedbackRect that samples the same rectangle of the texture, inset by
     * mFeedbackInset at both corners. The title covers only the register setup, although
     * the routine also draws. Renderer::OnUnknownSlot8() calls it while g_nLsdMode is set.
     *
     * @ghidraAddress 0x0049ccb8
     */
    void SetupGsDrawContext();

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
     * Double buffer InitDisplayMode() builds at `0x006f36c0`, addressed through its uncached alias.
     *
     * GetReservedVramWords() reads the depth buffer page from it. +0x448
     */
    GsDoubleBuffer *mpDisplayBuffers;
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

private:
    // Draw a string in the debug stroke font. Each glyph is a six-point line strip from the table
    // at 0x006f2f28, scaled to a cell of rect.w by rect.h from a pen at rect.x and rect.y, in GS
    // primitive pixels. Letters of either case share one glyph, and '.' through '9' follow them.
    // Any other character draws nothing and advances the pen two cells, and a glyph advances it
    // one and a half. The caller opens a REGLIST tag of PRIM, RGBAQ, and six XYZ2 first. The
    // routine writes through g_gfxDevice rather than a receiver. 0x0049bc20.
    static void DrawDebugText(const char *pszText, const Rect &rect, const Color &color);

    // Draw one flat sprite over rect, in GS primitive pixels. The caller opens a REGLIST tag of
    // PRIM, RGBAQ, and two XYZ2 first. The routine writes through g_gfxDevice rather than a
    // receiver. 0x0049c630.
    static void DrawTimingBar(const Rect &rect, const Color &color);

    // Average the frame time and the sync time over five frames and print "fps %d sync %d" near the
    // top right corner. No caller survives. 0x0049c388.
    void DrawFpsReadout();

    // Frame-rate sampling for DrawFpsReadout().
    int mnFpsCountdown;  // +0x454
    float mflFrameMsSum; // +0x458
    int mnFps;           // +0x45c
    float mflSyncMsSum;  // +0x460
    int mnSyncMsAverage; // +0x464
    int mUnknown468;     // +0x468

public:
    /**
     * Rectangle SetupGsDrawContext() draws and samples, in fractions of the display.
     *
     * Renderer::OnUnknownSlot8() writes it directly, with no accessor, before each call. +0x46c
     */
    Rect mFeedbackRect;
    /**
     * Blend factor SetupGsDrawContext() writes into ALPHA_1 FIX after scaling it by 128.
     *
     * Renderer::OnUnknownSlot8() writes it directly. +0x47c
     */
    float mFeedbackAlpha;
    /**
     * Inset of the texture window at both corners, in sixteenths of a texel.
     *
     * Renderer::OnUnknownSlot8() writes it directly. +0x480
     */
    int mFeedbackInset;

private:
    unsigned char mReserved484[0x0c]; // +0x484
    // Colour SetClearColor() last recorded.
    Color mClearColor; // +0x490
};

/**
 * The display device.
 *
 * @ghidraAddress 0x006f2a80
 */
extern GfxDevice g_gfxDevice;
