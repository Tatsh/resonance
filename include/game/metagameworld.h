#pragma once

#include "game/rawcontroller.h"

class InputCheatDetectorMet;
class RendererBase;

/**
 * Owner of the front-end world, outside a game session.
 *
 * `13MetaGameWorld` in the RTTI descriptor at `0x00901c90`, with RawController as its one public
 * base at offset 0. The object is 0xc bytes, which the allocation in GameManagerImpl::Start()
 * fixes, so the inherited vptr at `+0x00` is followed by two members of its own. Its vtable at
 * `0x00810f58` has three entries and a zero terminator at index 3, the type function at
 * `0x003d45e8`, the destructor at `0x003d4790`, and the RawController override below.
 *
 * The world owns the front-end renderer at `+0x04` and a cheat detector at `+0x08`. A controller
 * reading reaches the detector first and then the renderer, as a RawControllerMsg.
 *
 * GameManagerImpl drives the world through GetRenderer() and OnUnknownForwarder003d4890(). No
 * caller of OnUnknownForwarder003d4860() or OnUnknownQuery003d48c0() is recovered.
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
     * @param nUnknown1 The first word of the reading.
     * @param nUnknown2 The second word of the reading.
     * @param nUnknown3 The third word of the reading.
     * @param flUnknown4 The float of the reading.
     * @ghidraAddress 0x003d3288
     */
    virtual void OnUnknownSlot2(int nUnknown1, int nUnknown2, int nUnknown3, float flUnknown4);

    /**
     * Report the front-end renderer.
     *
     * An out-of-line accessor with eighteen callers, among them GameManagerImpl::DrawFrame() and
     * the script cheats, several of which cast the result to MetRenderer. The body is
     * byte-identical to every other two-instruction accessor of a pointer at `+0x04`, which is why
     * the program titled it after one of them.
     *
     * @return The renderer.
     * @ghidraAddress 0x003d4858
     */
    RendererBase *GetRenderer();

    /**
     * Unrecovered. Runs RendererBase slot 4 on the renderer.
     *
     * No caller is recovered.
     *
     * @ghidraAddress 0x003d4860
     */
    void OnUnknownForwarder003d4860();

    /**
     * Unrecovered. Runs RendererBase slot 5 on the renderer.
     *
     * GameManagerImpl::OnBeginGameLocal(), GameManagerImpl::OnUnpauseGameSystem(), and
     * GameManagerImpl::StartPlayback() call it.
     *
     * @ghidraAddress 0x003d4890
     */
    void OnUnknownForwarder003d4890();

    /**
     * Unrecovered. Report MetRenderer's word at `+0x60`, or 0 under the null renderer.
     *
     * The body returns 0 when QueryConfigFlag() reports option 0xcb set, the option that selects
     * MetNullRenderer. Otherwise it converts the renderer back to its MetRenderer and reads the
     * word. No caller is recovered.
     *
     * @return The word, or 0.
     * @ghidraAddress 0x003d48c0
     */
    int OnUnknownQuery003d48c0();

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
