#pragma once

#include "app/rendererbase.h"
#include "game/gameparams.h"

class Message;

/**
 * Front-end renderer that draws nothing, chosen in place of MetRenderer by a configuration option.
 *
 * `15MetNullRenderer` in the RTTI descriptor at `0x008eff00`, with RendererBase as its one public
 * base at offset 0. MetaGameWorld::CreateRenderer() allocates it at 0x88 bytes under the
 * `MsgSink` tag when PythonEvt::QueryOption() reports option 0xcb set. Its vtable at
 * `0x00801d58` has eleven entries, the same length as RendererBase's. Against that table it
 * overrides the destructor and slots 3, 7, and 8, the three RendererBase leaves pure, which is
 * what makes the class concrete. Slots 4 and 5 hold two-instruction empty bodies of its own at
 * `0x00311a10` and `0x00311a18`, the base's empty bodies re-emitted rather than overrides, so they
 * are not declared.
 *
 * The constructor runs RendererBase's, constructs the GameParams at `+0x48`, clears `+0x80`, and
 * then calls a run of front-end set-up routines ending in the stage-list rebuild at `0x003cc7a0`.
 * The destructor calls MetFreqMakerAssetManager::Destroy() at `0x002551b8` through its out-of-line
 * forwarder at `0x00254970`, destroys the GameParams, and runs RendererBase's teardown.
 *
 * Only the declarations are written.
 */
class MetNullRenderer : public RendererBase {
public:
    /**
     * @ghidraAddress 0x0030e2c8
     */
    MetNullRenderer();

    /**
     * @ghidraAddress 0x0030e400
     */
    virtual ~MetNullRenderer();

    /**
     * Handle one message. MsgSink slot 3, pure in RendererBase.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00311ff0
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Unrecovered. RendererBase slot 7, pure in the base, with an empty body here.
     *
     * @ghidraAddress 0x00311a20
     */
    virtual void OnUnknownSlot7();

    /**
     * Unrecovered. RendererBase slot 8, pure in the base.
     *
     * @ghidraAddress 0x0030f320
     */
    virtual void OnUnknownSlot8();

private:
    GameParams mUnknown48; // +0x48
    // Cleared by the constructor. Slot 8 tests it and clears it again.
    int mUnknown80; // +0x80
    // A count slot 8 loops up to.
    int mUnknown84; // +0x84
};
