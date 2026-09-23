#pragma once

#include "msg/cmdmsg.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `18EnableFreestyleMsg` in the RTTI descriptor at `0x008f09e0`, with CmdMsg as its one base. The
 * object is 0x10 bytes and its vtable is at `0x007e4570`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). Both members are public because
 * Gamer's freestyle handler at `0x00111080` reads them directly with no accessor in the image. It
 * dispatches Player slot 4 on mPlayer to find the track, runs the freestyle effect over mBar to
 * mBar + 8, and sets CmdMsg::mUnknown04 to 1.
 *
 * The word at `+0x04` belongs to CmdMsg, which New() zeroes. The two words after it belong to this
 * class, for the reason CmdMsg records.
 */
class EnableFreestyleMsg : public CmdMsg {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 414.
     *
     * @return The message.
     * @ghidraAddress 0x003d7730
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001ca8c8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nEnableFreestyleMsgType.
     * @ghidraAddress 0x001ca930
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `EnableFreestyleMsg`.
     * @ghidraAddress 0x001ca940
     */
    virtual const char *Name();

    int mBar;        /*!< The bar freestyle starts at. +0x08 */
    Player *mPlayer; /*!< The player freestyle is enabled for. +0x0c */
};

/**
 * Identity that EnableFreestyleMsg::Type() reports.
 *
 * This word belongs to EnableFreestyleMsg because EnableFreestyleMsg::Type() at `0x001ca930`
 * returns it, and the registration at `0x003d9818` passes the same value, 414, as the identity of
 * this class's factory.
 *
 * @ghidraAddress 0x006d0314
 */
extern int g_nEnableFreestyleMsgType;
