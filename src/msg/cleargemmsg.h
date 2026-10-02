#pragma once

#include "mid/mbt.h"
#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `11ClearGemMsg` in the RTTI descriptor at `0x008ef7f0`, with Message as its one base. The object
 * is 0x10 bytes and its vtable is at `0x007e2818`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). Every member is public because
 * AppTunnel::HandleMessage() at `0x00449790` reads them directly with no accessor in the image. It
 * converts mPosition's tick and mGem to floats and loads mTrack with `lb`, which reads only the low
 * byte of the word.
 *
 * The destructor at `0x001bf830` is compiler-generated and has no declaration here.
 */
class ClearGemMsg : public Message {
public:
    /**
     * Construct a message with the position at kMBTInfinity and the rest unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    ClearGemMsg() {
    }

    /**
     * Clear one gem.
     *
     * Inline, with no address of its own. PhraseMgr::AddGem() expands it on its stack at
     * `0x001bad0c` with an IsFiniteMBT-checked position and the manager's track.
     *
     * @param position The song position of the gem.
     * @param nTrack The track.
     * @param nGem The gem.
     */
    ClearGemMsg(Mid::MBT position, int nTrack, int nGem)
        : mPosition(position), mTrack(nTrack), mGem(nGem) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 404.
     *
     * @return The message.
     * @ghidraAddress 0x003d74b8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001bf8e0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nClearGemMsgType.
     * @ghidraAddress 0x001bf938
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `ClearGemMsg`.
     * @ghidraAddress 0x001bf948
     */
    virtual const char *Name();

    Mid::MBT mPosition; /*!< The song position of the gem to clear. +0x04 */
    int mTrack;         /*!< The track. +0x08 */
    int mGem;           /*!< The gem. +0x0c */
};

/**
 * Identity that ClearGemMsg::Type() reports.
 *
 * This word belongs to ClearGemMsg because ClearGemMsg::Type() at `0x001bf938` returns it, and the
 * registration at `0x003d9818` passes the same value, 404, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d02c4
 */
extern int g_nClearGemMsgType;
