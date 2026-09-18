#pragma once

/**
 * Owner of the running game.
 *
 * `15GameManagerImpl` in the RTTI descriptor at `0x008f0090`, deriving from `11GameManager` at
 * `0x008ef6b0`, which derives in turn from MsgSink. Recovery has barely started. The object is
 * 0x10c bytes, its constructor is at `0x00105f50`, and its table at `0x007cd5f8` runs 36 slots.
 * Neither base is declared yet, because nothing in the application layer uses either one.
 *
 * This declaration exists to satisfy the references from Globals, which creates the single
 * instance in Init(), and from Application::Run() and MainLoop, which drive the four slots below.
 */
class GameManagerImpl {
public:
    virtual ~GameManagerImpl();

    /**
     * Start the game.
     *
     * Vtable slot 13.
     *
     * @ghidraAddress 0x0010c168
     */
    void Start();

    /**
     * Draw one frame.
     *
     * Vtable slot 4.
     *
     * @ghidraAddress 0x001065a8
     */
    void DrawFrame();

    /**
     * Refresh the display without a full frame.
     *
     * Vtable slot 5. MainLoop uses this to keep the screen alive during a long operation.
     *
     * @ghidraAddress 0x0010bfa0
     */
    void DrawFrameSimple();

    /**
     * Advance the sound banks.
     *
     * Vtable slot 14. One of MainLoop's two periodic timers drives this.
     *
     * @ghidraAddress 0x00106e28
     */
    void PollBanks();
};
