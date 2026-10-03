#pragma once

#include "os/asynccallback.h"
#include "synth/callbackxferbdtoiop.h"

/**
 * Receiver that moves an HD sound bank into the sound driver in one transfer.
 *
 * Its RTTI descriptor is at `0x008eeee8`. It has AsyncCallback as its one public base at offset 0.
 * The vtable at `0x0081cf80` has the base's three slots and no more. The class does not add a
 * virtual or declare a destructor. An instance is 8 bytes.
 *
 * Unlike the BD transfer this is not chunked. The one completion moves the whole buffer to the IOP,
 * releases it, and clears the flag that a BD transfer defers to, then resumes the deferred BD
 * transfer when there is one.
 */
class CallbackXferHdToIop : public AsyncCallback {
public:
    /**
     * Move the bank that has just been read and release the buffer.
     *
     * Reports `HD bank loading returned async error %d` for a positive status and skips the
     * transfer, but still releases the buffer and clears the flag.
     *
     * @param nHandle The identifier the read was queued under.
     * @param nFile The file the read was issued against.
     * @param pBuffer The destination the read filled.
     * @param nLength The number of bytes the read requested.
     * @param nStatus Zero once the data is in place, or a positive failure code.
     * @ghidraAddress NTSC-U/C: 0x00464d10
     * @ghidraAddress PAL: 0x004a26d0
     */
    virtual void Done(int nHandle, int nFile, void *pBuffer, int nLength, int nStatus);

    /*!< Transfer to resume once this one reports, or null. Public because the routine that starts a
         BD transfer writes it directly at `0x00462080`, and the image supplies no setter for it.
         +0x04 */
    CallbackXferBdToIop *mpBdXfer;
};

/**
 * The one HD transfer receiver, constructed by the static initialiser at `0x00464170`.
 *
 * The original had internal linkage, because the whole module compiled as one translation unit
 * named midi_main.cpp. Declaring it here is what splitting the module into a file per class costs.
 *
 * @ghidraAddress NTSC-U/C: 0x006e9bc8
 * @ghidraAddress PAL: 0x0072d588
 */
extern CallbackXferHdToIop g_hdXfer;
