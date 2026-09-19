#pragma once

/**
 * Owner of the display, the GIF packet buffer, and the frame timing counters.
 *
 * The class is not polymorphic and has no RTTI. Its name is inferred from its methods, which
 * build GIF packets, set GS registers, and flip the framebuffer. Its members have not been
 * recovered beyond the display fields that Init() writes.
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
     * that would not change the masked bits is dropped, which is why callers pass a mask covering
     * only the field they mean to set rather than the whole register.
     *
     * @param nReg The GS register number.
     * @param qwValue The value to set, of which only the masked bits are used.
     * @param qwMask The bits of the register this call owns.
     * @ghidraAddress 0x0049ffd0
     */
    void SetGsReg(int nReg, unsigned long long qwValue, unsigned long long qwMask);

    /**
     * Make room for a GIF packet of the given size.
     *
     * @param nQuadwords The space to reserve.
     * @ghidraAddress 0x0049fd98
     */
    void ReserveGifSpace(int nQuadwords);

    /** Non-zero while geometry is submitted through VU1 rather than the software path. +0x44c */
    int mnUseVu1;
};

/**
 * The display device.
 *
 * @ghidraAddress 0x006f2a80
 */
extern GfxDevice g_gfxDevice;
