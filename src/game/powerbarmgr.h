#pragma once

#include "app/msgsource.h"

/**
 * Source of the powerbar a bar of one track awards when its phrase is caught.
 *
 * `11PowerbarMgr` in the RTTI, with MsgSource as its one base at offset 0 and no members of its
 * own, so the object is the 0x14-byte MsgSource subobject. Its table at `0x007e2c70` keeps
 * MsgSource's AddSink() and RemoveSink() and adds GetPowerbar() at slot 4, which addresses the
 * shared pure-virtual stub.
 *
 * PhraseMgr creates the one it keeps at `+0x2c`, and PhraseMgr::GetPowerbar() forwards to it.
 *
 * The constructor and the destructor are implicitly declared. The constructor is emitted at
 * `0x001c0680` in PhraseMgr's unit and again at `0x001c6058` in GamePowerbarMgr's, and the
 * deleting destructor likewise at `0x001c0848` and `0x001c6390`.
 */
class PowerbarMgr : public MsgSource {
public:
    /**
     * Report the powerbar a bar awards.
     *
     * @param nBar The bar, already mapped through the play map.
     * @return The powerbar, or -1 for none.
     */
    virtual int GetPowerbar(int nBar) = 0;
};
