#pragma once

#include "msg/cmdmsg.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008f09e0`. It has CmdMsg as its one base. The object is 0x10 bytes
 * and its vtable is at `0x007e4570`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(). Both members are public because
 * Gamer's freestyle handler at `0x00111080` reads them directly with no accessor in the image. It
 * dispatches Player slot 4 on mPlayer to find the track, runs the freestyle effect over mBar to
 * mBar + 8, and sets CmdMsg::mResult to 1.
 *
 * The word at `+0x04` belongs to CmdMsg, which New() zeroes. The two words after it belong to this
 * class, for the reason CmdMsg records.
 *
 * The destructor at `0x001ca7e8` is compiler-generated and has no declaration here.
 */
class EnableFreestyleMsg : public CmdMsg {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 414.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7730
     * @ghidraAddress PAL: 0x0040f630
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x001ca8c8
     * @ghidraAddress PAL: 0x001d0780
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nEnableFreestyleMsgType.
     * @ghidraAddress NTSC-U/C: 0x001ca930
     * @ghidraAddress PAL: 0x001d07e8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `EnableFreestyleMsg`.
     * @ghidraAddress NTSC-U/C: 0x001ca940
     * @ghidraAddress PAL: 0x001d07f8
     */
    virtual const char *GetName() const;

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
 * @ghidraAddress NTSC-U/C: 0x006d0314
 * @ghidraAddress PAL: 0x00713aac
 */
extern int g_nEnableFreestyleMsgType;
