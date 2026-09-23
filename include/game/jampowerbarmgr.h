#pragma once

#include "game/powerbarmgr.h"

/**
 * Powerbar source for the modes that award no powerbars.
 *
 * `14JamPowerbarMgr` in the RTTI, with PowerbarMgr as its one base and no members of its own. Its
 * table is at `0x007e2c40`. PhraseMgr creates one for every play mode and track kind that does
 * not get a GamePowerbarMgr.
 *
 * The constructor at `0x001bfbc0` and the deleting destructor at `0x001c06b8` are implicitly
 * declared and emitted in PhraseMgr's unit, which is also where the inline GetPowerbar() is
 * emitted.
 */
class JamPowerbarMgr : public PowerbarMgr {
public:
    /**
     * @param nBar The bar. Not read.
     * @return -1, for no powerbar.
     * @ghidraAddress 0x001c07f0
     */
    virtual int GetPowerbar([[maybe_unused]] int nBar) {
        return -1;
    }
};
