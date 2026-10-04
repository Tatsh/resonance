#pragma once

#include "met/metrenderer.h"
#include "rnd/mesh.h"

class FadeUser;

/**
 * Driver of the full-screen metagame fade.
 *
 * The class emits no RTTI descriptor and no `__FILE__` path survives for its translation unit,
 * because its constructor writes no virtual function table pointer and it declares no destructor.
 * The name here is inferred from the two literals its constructor and its fade routines use,
 * `metfade.rect` at `0x007d8dd0` and `met_fade.view` at `0x007d8de0`.
 *
 * The global `operator new(0x2c)` at its allocation sites fixes the object size at 0x2c bytes.
 * MetLoadGameScreen and MetMemDetectStartup each build one and each release it through the scalar
 * deallocator with no null test. A delete expression compiles to the same call for a class with no
 * destructor. The European release also builds one in MetLogoScreen.
 *
 * A fade runs from a start frame to an end frame. While one runs, the fade view is attached to the
 * renderer's screen scene and the rectangle's vertex alpha follows a linear ramp over the span.
 * When it finishes, the receiver passed to FadeOut() or FadeIn() is notified. No member name is
 * attested anywhere in the image, so every identifier below is inferred.
 *
 * The European release detaches the view before it attaches it at the start of a fade. A finished
 * fade sets the final alpha before it hides the rectangle, and the rectangle stays hidden.
 */
class MetFade {
public:
    /** The fade in progress. */
    enum State {
        kStateIdle = 0, /*!< No fade is running. */
        kStateOut = 1,  /*!< FadeOut() is running. */
        kStateIn = 2,   /*!< FadeIn() is running. */
    };

    /**
     * Resolve the fade rectangle and prime the two timers.
     *
     * Both start frames begin at 1.0e9, which is the sentinel for a fade that is not running. The
     * rectangle is the object named `metfade.rect`, cast to Rnd::Mesh, and a null result is stored
     * as such. The constructor then hides the rectangle without testing it for null.
     *
     * @param pRenderer The front-end renderer the fade draws through.
     * @ghidraAddress NTSC-U/C: 0x0016a1c8
     * @ghidraAddress PAL: 0x0016c3c8
     */
    MetFade(MetRenderer *pRenderer);

    /**
     * Start the fade out, the one a screen runs as it departs.
     *
     * Shows the rectangle, records the span and the ramp, and attaches `met_fade.view` to the
     * renderer's screen scene. The ramp runs the rectangle's alpha from 1 at the start frame down
     * to 0 at the end frame.
     *
     * @param duration The length of the fade in frames.
     * @param start The frame the fade starts on.
     * @param pUser The receiver whose slot 2 runs when the fade finishes, or null for none.
     * @param nRetainView Stored for UpdateIn(). The fade out itself never reads it.
     * @ghidraAddress NTSC-U/C: 0x0016a608
     * @ghidraAddress PAL: 0x0016ce48
     */
    void FadeOut(float duration, float start, FadeUser *pUser, int nRetainView);

    /**
     * Start the fade in, the one a screen runs as it enters.
     *
     * The same as FadeOut() against the second timer, except that the rectangle is not shown and
     * the ramp runs the alpha from 0 at the start frame up to 1 at the end frame.
     *
     * @param duration The length of the fade in frames.
     * @param start The frame the fade starts on.
     * @param pUser The receiver whose slot 3 runs when the fade finishes, or null for none.
     * @param nRetainView Non-zero to leave the view attached and the rectangle shown when the
     * fade finishes.
     * @ghidraAddress NTSC-U/C: 0x0016d750
     * @ghidraAddress PAL: 0x0016cc30
     */
    void FadeIn(float duration, float start, FadeUser *pUser, int nRetainView);

    /**
     * Advance whichever fade is running.
     *
     * Any state other than kStateIdle and kStateOut advances the fade in.
     *
     * @param frame The current frame.
     * @ghidraAddress NTSC-U/C: 0x0016d710
     * @ghidraAddress PAL: 0x001700e8
     */
    void Update(float frame);

#ifdef VIDEO_STANDARD_PAL
    /**
     * Detach the fade view from the screen scene and hide the rectangle.
     *
     * The routine has no callers. The name is inferred.
     *
     * @ghidraAddress PAL: 0x0016c910
     */
    void Detach();

    /**
     * Attach the fade view at the end of the screen scene and show the rectangle opaque black.
     *
     * The view is detached first. The routine has no callers. The name is inferred.
     *
     * @ghidraAddress PAL: 0x0016ca20
     */
    void ShowOpaque();

    /**
     * Report how far the rectangle covers the screen.
     *
     * The routine has no callers. The name is inferred.
     *
     * @return 0 while the rectangle is hidden, otherwise 1 less the alpha of the first vertex.
     * @ghidraAddress PAL: 0x00170128
     */
    float GetCoverage() const;
#endif

private:
    /**
     * Darken the rectangle for one frame of the fade out.
     *
     * At the end frame it removes the fade view, hides the rectangle, and notifies the user through
     * OnFadeOutDone(). The European release also clears g_nFadeRunning. The North American release
     * shows the rectangle again after hiding it.
     *
     * @param frame The current frame.
     * @ghidraAddress NTSC-U/C: 0x0016a2d0
     * @ghidraAddress PAL: 0x0016c4f0
     */
    void UpdateOut(float frame);
    /**
     * Lighten the rectangle for one frame of the fade in.
     *
     * At the end frame it removes the fade view and hides the rectangle unless the view is
     * retained, and notifies the user through OnFadeInDone(). The European release also clears
     * g_nFadeRunning. The North American release shows the rectangle again after hiding it.
     *
     * @param frame The current frame.
     * @ghidraAddress NTSC-U/C: 0x0016a468
     * @ghidraAddress PAL: 0x0016c708
     */
    void UpdateIn(float frame);

    float outStart_; // 1.0e9 while no fade out runs.
    float outEnd_;
    float inStart_; // 1.0e9 while no fade in runs.
    float inEnd_;
    float rate_;   // The slope of the alpha ramp, one over the span.
    float offset_; // The ramp's value at frame 0.
    int retainView_;
    Rnd::Mesh *rect_; // The object named `metfade.rect`.
    int state_;       // A State value.
    FadeUser *user_;
    MetRenderer *renderer_;
};
