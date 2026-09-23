#pragma once

#include "app/tnltrigger.h"

class AppTunnel;

/**
 * A panel effect AppTunnel starts on one ring section at a later frame.
 *
 * `15TnlPanelFXDelay` in the RTTI descriptor, deriving from TnlTrigger. Its type function is at
 * `0x00457180` and its vtable at `0x0081b9c0`. The object is 0x14 bytes. The destructor at
 * `0x00457150` is compiler-generated. AppTunnel builds one per bar of a panel run at `0x004481d0`
 * and `0x00448d58`, with the constructor inlined.
 */
class TnlPanelFXDelay : public TnlTrigger {
public:
    /**
     * Record the section to start the panel effect on.
     *
     * @param pTunnel The tunnel that runs the effect.
     * @param nRing The ring of the section.
     * @param nSlice The slice of the section.
     * @param nForward Non-zero to rise along the section offset.
     */
    TnlPanelFXDelay(AppTunnel *pTunnel, int nRing, int nSlice, int nForward)
        : mRing(nRing), mSlice(nSlice), mForward(nForward), mTunnel(pTunnel) {
    }

    /**
     * Start an idle panel effect on the recorded section.
     *
     * The result of AppTunnel::StartPanelFX() is discarded, so a run with no idle panel drops the
     * effect.
     *
     * @ghidraAddress 0x004571f8
     */
    virtual void Fire();

private:
    int mRing;          // +0x04
    int mSlice;         // +0x08
    int mForward;       // +0x0c
    AppTunnel *mTunnel; // +0x10
};
