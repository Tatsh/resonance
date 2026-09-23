#pragma once

#include <libgraph.h>

/**
 * The display and draw environments of a double-buffered framebuffer.
 *
 * This is the game's own structure rather than the SDK's sceGsDBuff. The SDK setter at
 * `0x005e4b30` takes its arguments as `(psm, w, h, ztest, zpsm, clear)` and fills a 0x230-byte
 * sceGsDBuff, whereas SetDefaults() takes `(w, h, psm, ztest, zpsm, clear)` and fills a structure
 * whose draw halves begin at +0x80 and +0x170 and which records its parameters from +0x268 on. The
 * extra room at the front comes from each display environment recording a second pair of display
 * registers for read circuit 1 beside the SDK's pair for read circuit 2.
 *
 * The two read circuits show the same buffer one line apart, and PMODE blends them at half
 * strength. SetDispEnvs() builds both pairs.
 *
 * The class is not polymorphic and has no RTTI, and no literal in the image identifies it. Its
 * name and the names of its members are inferred from what the routines below do with it.
 * GfxDevice addresses the one instance at an uncached address and reads the draw environments and
 * the depth buffer page directly with no accessor. The data members are therefore public.
 */
class GsDoubleBuffer {
public:
    /** One display environment, 0x40 bytes. */
    struct DispEnv {
        /** The SDK environment, which PutDispEnv() writes to read circuit 2. +0x00 */
        sceGsDispEnv mEnv;
        /** DISPFB1, the circuit 1 frame buffer. +0x28 */
        unsigned long long mDispFb1;
        /** DISPLAY1, the circuit 1 display rectangle. +0x30 */
        unsigned long long mDisplay1;
        // The stride between the two environments is 0x40, and nothing reads this word.
        unsigned long long mReserved38; // +0x38
    };

    /** One draw environment preceded by its GIFtag and followed by its clear, 0xf0 bytes. */
    struct DrawHalf {
        /** A+D tag whose loop count PutDrawEnv() sets to include or exclude the clear. +0x00 */
        sceGifTag mGifTag;
        /** FRAME_1, ZBUF_1, XYOFFSET_1, and the rest of the context 1 registers. +0x10 */
        sceGsDrawEnv1 mDraw;
        /** The register writes that clear the buffer. +0x90 */
        sceGsClear mClear;
    };

    /**
     * Fill both halves for a framebuffer and a depth buffer laid out from page 0.
     *
     * Records the geometry, places buffer 0 at page 0, buffer 1 after it, and the depth buffer
     * after both, then builds the display and draw environments. The page count assumes 4 bytes
     * per pixel for any format other than PSMCT24 and PSMCT16.
     *
     * @param nWidth Width in pixels.
     * @param nHeight Height in pixels.
     * @param nPsm Pixel storage format of the frame buffers.
     * @param nZTest Depth test, where zero also masks depth writes.
     * @param nZPsm Pixel storage format of the depth buffer.
     * @param nClear Passed through to SetDrawEnvs(), which never reads it.
     * @ghidraAddress 0x0058e9b0
     */
    void
    SetDefaults(short nWidth, short nHeight, short nPsm, short nZTest, short nZPsm, short nClear);

    /**
     * Build both display environments.
     *
     * Each takes the SDK defaults with PMODE blending at 0x80 and both circuits enabled. Circuit
     * 1 shows the buffer as the SDK set it, and circuit 2 starts one line lower, one line shorter,
     * and two units to the right.
     *
     * @param nWidth Width in pixels.
     * @param nHeight Height in pixels.
     * @param nPsm Pixel storage format.
     * @param nFbp0 Page of buffer 0.
     * @param nFbp1 Page of buffer 1.
     * @ghidraAddress 0x0058e330
     */
    void SetDispEnvs(short nWidth, short nHeight, short nPsm, short nFbp0, short nFbp1);

    /**
     * Build both draw environments, their GIFtags, and their clears.
     *
     * @param nWidth Width in pixels.
     * @param nHeight Height in pixels.
     * @param nPsm Pixel storage format of the frame buffers.
     * @param nFbp0 Page of buffer 0.
     * @param nFbp1 Page of buffer 1.
     * @param nZbp Page of the depth buffer.
     * @param nZTest Depth test, where zero also masks depth writes.
     * @param nZPsm Pixel storage format of the depth buffer.
     * @param nClear Never read. The caller passes it in the tenth argument slot on the stack.
     * @ghidraAddress 0x0058e538
     */
    void SetDrawEnvs(short nWidth,
                     short nHeight,
                     short nPsm,
                     short nFbp0,
                     short nFbp1,
                     short nZbp,
                     short nZTest,
                     short nZPsm,
                     short nClear);

    /**
     * Send one draw environment to the GS through the SDK.
     *
     * Both GIFtags receive the loop count first. The choice of clear therefore persists into the
     * other half.
     *
     * @param nHalf Zero for half 0, anything else for half 1.
     * @param bClear Non-zero to send the clear with the environment.
     * @ghidraAddress 0x0058e7e8
     */
    void PutDrawEnv(int nHalf, int bClear);

    /**
     * Write one display environment to the privileged GS registers.
     *
     * Read circuit 2 is always enabled. Circuit 1 is enabled or disabled by bEnableCircuit1, and
     * the choice is stored back into the environment.
     *
     * @param nHalf Zero for half 0, anything else for half 1.
     * @param bEnableCircuit1 Non-zero to enable read circuit 1.
     * @ghidraAddress 0x0058e860
     */
    void PutDispEnv(int nHalf, int bEnableCircuit1);

    /** Display environments of the two halves. +0x000 */
    DispEnv mDisp[2];
    /** Draw environments of the two halves. +0x080 */
    DrawHalf mHalves[2];
    // Nothing in the image reads or writes this span.
    unsigned char mReserved260[8]; // +0x260
    /** Width SetDefaults() recorded. +0x268 */
    short mWidth;
    /** Height SetDefaults() recorded. +0x26a */
    short mHeight;
    /** Frame buffer pixel storage format SetDefaults() recorded. +0x26c */
    short mPsm;
    /** Page of buffer 0, always zero. +0x26e */
    short mFbp0;
    /** Page of buffer 1, the pages one buffer occupies. +0x270 */
    short mFbp1;
    /** Depth buffer pixel storage format SetDefaults() recorded. +0x272 */
    short mZPsm;
    /**
     * Page of the depth buffer, twice mFbp1.
     *
     * GfxDevice::GetReservedVramWords() reads it to find where the buffers end. +0x274
     */
    short mZbp;
};
