#pragma once

#include <vector>

#include "app/tnlemitter.h"

namespace Rnd {
class View;
} // namespace Rnd

/**
 * Burst of three particle arms shown for a limited time after a trigger frame.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the objects it drives, "arms.view" and "arms0.ps" through
 * "arms2.ps".
 *
 * The constructor detaches "arms.view" from the four local player views and silences the three
 * systems. Start() shows the view and restarts the systems. For the next 1920 frames the view
 * animates with the song, for the 7680 after that it animates while the systems stay silent, and
 * then the view hides.
 *
 * AppTunnel allocates one, 0x14 bytes, and stores it at `+0x18`. The destructor at `0x0043f920`
 * is the implicit one and is not written.
 */
class TnlArms {
public:
    /**
     * Resolve the view and the three systems.
     *
     * @ghidraAddress 0x0043f3f8
     */
    TnlArms();

    /**
     * Show the view and restart the systems from flFrame.
     *
     * AppTunnel inlines this, and the out-of-line copy has no caller.
     *
     * @param flFrame The trigger frame.
     * @ghidraAddress 0x00456ad0
     */
    void Start(float flFrame);

    /**
     * Drive the view to flFrame and end the burst on schedule.
     *
     * Returns at once before the trigger frame. AppTunnel inlines this at `0x00446ff4`, and the
     * out-of-line copy has no caller.
     *
     * @param flFrame The current frame.
     * @ghidraAddress 0x00456ba0
     */
    void SetFrame(float flFrame);

private:
    // AppTunnel's WinMsg handler draws mView in each winner's local view.
    friend class AppTunnel;

    float mStartFrame; // Trigger frame, 1e9 when idle.
    Rnd::View *mView;  // "arms.view".
    std::vector<TnlEmitter> mEmitters;
};
