#ifndef LIBPAD_H
#define LIBPAD_H

// Build support for the open-source SDK. ps2sdk's pad library names its entry points differently
// from Sony's libpad. The entry point the reconstruction calls is declared here with the signature
// the shipped program's body and call site prove. Nothing here is reconstructed source.

#ifdef __cplusplus
extern "C" {
#endif

// Copies the latest report of the pad at nPort and nSlot into pData and returns a positive value
// when a report was copied. Bytes 2 and 3 of a report are the button bits, active low.
int scePadRead(int nPort, int nSlot, unsigned char *pData);

// Starts the pad library. PadRecord::Open() passes 0, once per run.
int scePadInit(int nMode);

// Opens the pad at nPort and nSlot with pDmaArea as its 256-byte, 64-byte-aligned DMA area.
int scePadPortOpen(int nPort, int nSlot, void *pDmaArea);

// Closes the pad at nPort and nSlot.
int scePadPortClose(int nPort, int nSlot);

// Reports the pad's state. 0 is disconnected, 5 is busy, 6 is stable, and 99 is closed.
int scePadGetState(int nPort, int nSlot);

// Sends the six actuator bytes in pData to the pad at nPort and nSlot.
int scePadSetActDirect(int nPort, int nSlot, const unsigned char *pData);

// Reports one mode fact of the pad. PadRecord::Read() asks for the current identifier (1) and the
// extended identifier (2).
int scePadInfoMode(int nPort, int nSlot, int nTerm, int nOffset);

// Requests a main mode and a lock state. PadRecord::Read() asks for analog mode, locked.
int scePadSetMainMode(int nPort, int nSlot, int nOffset, int nLock);

// Reports the progress of the last request: 0 complete, 1 failed, 2 busy.
int scePadGetReqState(int nPort, int nSlot);

// Reports actuator facts. An actuator of -1 asks for the actuator count.
int scePadInfoAct(int nPort, int nSlot, int nActuator, int nTerm);

// Assigns each of the six actuators a byte of the direct actuator buffer.
int scePadSetActAlign(int nPort, int nSlot, const unsigned char *pAlign);

// Reports whether the pad supports pressure-sensitive buttons. The image leaves both entry points
// untitled at 0x0059d000 and 0x0059d060, and the names are Sony's, inferred from how
// PadRecord::Read() uses them.
int scePadInfoPressMode(int nPort, int nSlot);

// Switches the pad into pressure-sensitive mode.
int scePadEnterPressMode(int nPort, int nSlot);

#ifdef __cplusplus
}
#endif

#endif
