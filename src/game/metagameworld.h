#pragma once

#include "game/rawcontroller.h"

class InputCheatDetectorMet;
class RendererBase;

/**
 * Owner of the front-end world, outside a game session.
 *
 * Its RTTI descriptor is at `0x00901c90`. It has RawController as its one public base at offset 0.
 * The allocation in GameManagerImpl::Start() fixes the object at 0xc bytes. The inherited vptr at
 * `+0x00` is followed by two members. Its vtable at `0x00810f58` has three entries and a zero
 * terminator at index 3, the type function at `0x003d45e8`, the destructor at `0x003d4790`, and the
 * RawController override below.
 *
 * The world owns the front-end renderer at `+0x04` and a cheat detector at `+0x08`. A controller
 * reading reaches the detector first and then the renderer, as a RawControllerMsg.
 *
 * GameManagerImpl drives the world through GetRenderer() and StopFrontEnd(). No caller of
 * StartFrontEnd() or IsAwaitingStart() is recovered.
 *
 * The function at `0x003d4758` is the destructor of InputCheatDetectorMet, re-emitted in this
 * translation unit, and is recorded in that class.
 */
class MetaGameWorld : public RawController {
public:
    /**
     * Build the renderer and the cheat detector.
     *
     * Both members start null. CreateRenderer() runs next, and the detector is then allocated at
     * 8 bytes over g_metCheatSequences.
     *
     * @ghidraAddress 0x003d3110
     */
    MetaGameWorld();

    /**
     * Release the cheat detector through its own table, then the renderer through
     * DestroyRenderer().
     *
     * @ghidraAddress 0x003d4790
     */
    virtual ~MetaGameWorld();

    /**
     * Report a controller reading. Slot 2.
     *
     * The body forwards all four arguments unchanged to slot 2 of the cheat detector. It then
     * builds a RawControllerMsg on the stack whose reading is the four arguments in parameter
     * order and whose position is Mid::MBT(0), and hands it to the renderer's Handle().
     *
     * @param nTag The device tag, a four-character code.
     * @param nPadIndex The controller that produced the reading, from 1.
     * @param nButton The button or axis control number.
     * @param flValue The reading's value.
     * @ghidraAddress 0x003d3288
     */
    virtual void OnControllerReading(int nTag, int nPadIndex, int nButton, float flValue);

    /**
     * Report the front-end renderer.
     *
     * An out-of-line accessor with eighteen callers, among them GameManagerImpl::DrawFrame() and
     * the script cheats, several of which cast the result to MetRenderer. The body is
     * byte-identical to every other two-instruction accessor of a pointer at `+0x04`.
     *
     * @return The renderer.
     * @ghidraAddress 0x003d4858
     */
    RendererBase *GetRenderer();

    /**
     * Start the front end running through RendererBase::Start().
     *
     * No caller is recovered.
     *
     * @ghidraAddress 0x003d4860
     */
    void StartFrontEnd();

    /**
     * Stop the front end running through RendererBase::Stop().
     *
     * GameManagerImpl::OnBeginGameLocal(), GameManagerImpl::OnUnpauseGameSystem(), and
     * GameManagerImpl::StartPlayback() call it.
     *
     * @ghidraAddress 0x003d4890
     */
    void StopFrontEnd();

    /**
     * Report whether the title screen waits for Start, or 0 under the null renderer.
     *
     * The body returns 0 when QueryConfigFlag() reports option 0xcb set, the option that selects
     * MetNullRenderer. Otherwise it converts the renderer back to its MetRenderer and reads the
     * word at `+0x60`, which MetLogoScreen sets when it begins waiting and clears when Start or
     * Select is pressed. No caller is recovered.
     *
     * @return The word, or 0.
     * @ghidraAddress 0x003d48c0
     */
    int IsAwaitingStart();

private:
    /**
     * Build the front-end renderer.
     *
     * With option 0xcb set the renderer is a MetNullRenderer, and otherwise a MetRenderer, whose
     * RendererBase subobject lies at `+0x14`. The constructor is the one caller. GrooveWorld has
     * the routine of the same shape for the game renderer, which is where the name comes from.
     *
     * @ghidraAddress 0x003d31c0
     */
    void CreateRenderer();

    /**
     * Release the renderer through its own table and clear the member.
     *
     * The destructor is the one caller.
     *
     * @ghidraAddress 0x003d4810
     */
    void DestroyRenderer();

    // The front-end renderer, a MetNullRenderer or a MetRenderer. +0x04
    RendererBase *mRenderer;
    // Receives every controller reading ahead of the renderer. +0x08
    InputCheatDetectorMet *mCheatDetector;
};
