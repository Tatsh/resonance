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
};

/**
 * The display device.
 *
 * @ghidraAddress 0x006f2a80
 */
extern GfxDevice g_gfxDevice;
