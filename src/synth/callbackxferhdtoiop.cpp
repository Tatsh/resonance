#include "synth/callbackxferhdtoiop.h"

#include "os/log.h"
#include "os/mem.h"
#include "synth/midi_main.h"

// The tag the release below bills to. It is the module's original file rather than this one,
// because the whole of midi_main compiled as a single translation unit.
constexpr char kMidiMainFileName[] = "midi_main.cpp";

// The compiler generated the initialiser and destructor pair at 0x00464170 for this definition.
// 0x006e9bc8
CallbackXferHdToIop g_hdXfer;

// 0x00464d10
void CallbackXferHdToIop::Done([[maybe_unused]] int nHandle,
                               [[maybe_unused]] int nFile,
                               void *pBuffer,
                               int nLength,
                               int nStatus) {
    if (nStatus > 0) {
        LogPrintf("HD bank loading returned async error %d\n", nStatus);
    } else {
        XferToIop(g_nBankIopAddress, pBuffer, nLength);
    }
    MemFreeTagged(g_pHdXferBuffer, kMidiMainFileName, __LINE__);
    g_nHdXferInFlight = 0;
    if (mpBdXfer != nullptr && mpBdXfer->mRequestId != 0) {
        mpBdXfer->Resume();
    }
}
