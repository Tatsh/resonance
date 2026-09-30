#ifndef LIBPAD_H
#define LIBPAD_H

#ifdef __cplusplus
extern "C" {
#endif

/** Controller access through the padman server on the IOP. */

/** Size of the DMA area of one controller, in 16-byte units. */
#define scePadDmaBufferMax 16

/** States scePadGetState() reports. */
#define scePadStateDiscon 0   /*!< No controller is connected. */
#define scePadStateFindPad 1  /*!< The IOP is looking for a controller. */
#define scePadStateFindCTP1 2 /*!< The IOP is identifying the controller. */
#define scePadStateExecCmd 5  /*!< A command to the controller is running. */
#define scePadStateStable 6   /*!< The controller is ready. */
#define scePadStateError 7    /*!< The controller failed. */
#define scePadStateClosed 99  /*!< The port is not open. */

/** Progress scePadGetReqState() reports. */
#define scePadReqStateComplete 0 /*!< The last request finished. */
#define scePadReqStateFaild 1    /*!< The last request failed. */
#define scePadReqStateBusy 2     /*!< A request is running. */

/** Terms of scePadInfoMode(). */
#define InfoModeCurID 1     /*!< Identifier of the current mode. */
#define InfoModeCurExID 2   /*!< Extended identifier of the current mode. */
#define InfoModeCurExOffs 3 /*!< Offset of the current mode in the identifier table. */
#define InfoModeIdTable 4   /*!< An entry of the identifier table. */

/** Terms of scePadInfoAct(). */
#define InfoActFunc 1 /*!< Function of the actuator. */
#define InfoActSub 2  /*!< Sub-function of the actuator. */
#define InfoActSize 3 /*!< Size of the actuator's data. */
#define InfoActCurr 4 /*!< Current the actuator draws. */

/**
 * One of the two frames of a controller's DMA area. The IOP writes the two frames in turn.
 *
 * The frame with the larger count is the newer.
 */
typedef struct {
    unsigned char abData[32];         /*!< The latest report. */
    unsigned char abReserved32[16];   /*!< Undetermined. */
    unsigned char aabActInfo[4][4];   /*!< The four scePadInfoAct() terms of each actuator. */
    unsigned char aabCombInfo[4][4];  /*!< Count and members of each actuator combination. */
    unsigned short anModeTable[4];    /*!< Extended identifiers of the modes. */
    unsigned int nFrame;              /*!< Count of the frame. */
    unsigned int nReserved92;         /*!< Undetermined. */
    unsigned int nLength;             /*!< Size of the report in bytes. */
    unsigned char nModeConfig;        /*!< 1 when the mode tables are absent. */
    unsigned char nModeCurrentId;     /*!< Identifier of the current mode, in the high nibble. */
    unsigned char nModel;             /*!< Controller model. */
    unsigned char nReportReady;       /*!< Nonzero once a report was read. */
    unsigned char nModeCount;         /*!< Entries of anModeTable. */
    unsigned char nModeCurrentOffset; /*!< Index of the current mode in anModeTable. */
    unsigned char nActuatorCount;     /*!< Entries of aabActInfo. */
    unsigned char nCombinationCount;  /*!< Entries of aabCombInfo. */
    unsigned char abReserved108[4];   /*!< Undetermined. */
    unsigned char nState;             /*!< scePadState value. */
    unsigned char nReqState;          /*!< scePadReqState value. */
    unsigned char nCurrentTask;       /*!< 1 while the mode and actuator tables are valid. */
    unsigned char abReserved115[6];   /*!< Undetermined. */
    unsigned char abButtonMask[4];    /*!< Pressure-sensitive buttons, least significant first. */
    unsigned char abReserved125[3];   /*!< Undetermined. */
} scePadDmaFrame;

/**
 * Bind the pad servers, check the padman version, and reset the ports.
 *
 * @param nMode Passed to scePadPortInit().
 * @return The scePadPortInit() result, or zero on a version mismatch.
 */
int scePadInit(int nMode);

/**
 * Close every port record and reset padman.
 *
 * @param nMode Not used.
 * @return padman's result, or zero when the call failed.
 */
int scePadPortInit(int nMode);

/**
 * Stop padman.
 *
 * @return padman's result, or zero when the call failed.
 */
int scePadEnd(void);

/**
 * Start reading a controller into a DMA area.
 *
 * @param nPort Port.
 * @param nSlot Slot of a multitap, or zero.
 * @param pFrames Two frames aligned to 64 bytes. They must remain valid until the port closes.
 * @return padman's result, or zero when the area is misaligned, the port is open, or the call
 * failed.
 */
int scePadPortOpen(int nPort, int nSlot, scePadDmaFrame *pFrames);

/**
 * Stop reading a controller.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @return padman's result, or zero when the port is closed or the call failed.
 */
int scePadPortClose(int nPort, int nSlot);

/**
 * Pick the newer frame of a port's DMA area.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @return The frame.
 */
scePadDmaFrame *scePadGetDmaStr(int nPort, int nSlot);

/**
 * Report the count of the newer frame.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @return The count, or zero when the port is closed.
 */
unsigned int scePadGetFrameCount(int nPort, int nSlot);

/**
 * Copy the latest report of a controller.
 *
 * Bytes 2 and 3 of a report are the button bits, active low.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @param pData Receives the report, 32 bytes at most.
 * @return The report size, or zero when the port is closed.
 */
int scePadRead(int nPort, int nSlot, unsigned char *pData);

/**
 * Report the state of a controller.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @return A scePadState value. scePadStateExecCmd replaces scePadStateStable while a request
 * runs.
 */
int scePadGetState(int nPort, int nSlot);

/**
 * Write the name of a state.
 *
 * @param nState scePadState value.
 * @param pszName Receives the name, or an empty string for an unknown state.
 */
void scePadStateIntToStr(int nState, char *pszName);

/**
 * Overwrite the request progress of a controller.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @param nState scePadReqState value.
 * @return 1, or zero when the port is closed.
 */
int scePadSetReqState(int nPort, int nSlot, int nState);

/**
 * Report the request progress of a controller.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @return A scePadReqState value, or zero when the port is closed.
 */
int scePadGetReqState(int nPort, int nSlot);

/**
 * Write the name of a request progress value.
 *
 * @param nState scePadReqState value.
 * @param pszName Receives the name, or an empty string for a value above 3. The value 3 has no
 *                name, and the copy reads a null pointer.
 */
void scePadReqIntToStr(int nState, char *pszName);

/**
 * Describe an actuator of a controller.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @param nActuator Actuator index, or -1 for the actuator count.
 * @param nTerm InfoAct term.
 * @return The value, or zero when the tables are not valid.
 */
int scePadInfoAct(int nPort, int nSlot, int nActuator, int nTerm);

/**
 * Describe an actuator combination of a controller.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @param nList Combination index, or -1 for the combination count.
 * @param nOffset -1 for the member count, or the index of a member.
 * @return The value, or zero when the tables are not valid.
 */
int scePadInfoComb(int nPort, int nSlot, int nList, int nOffset);

/**
 * Describe the modes of a controller.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @param nTerm InfoMode term.
 * @param nOffset Table index for InfoModeIdTable, or -1 for the table size.
 * @return The value, or zero when the tables are not valid or a request runs.
 */
int scePadInfoMode(int nPort, int nSlot, int nTerm, int nOffset);

/**
 * Request a mode change.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @param nOffset Index of the mode in the identifier table.
 * @param nLock 3 to lock the mode against the controller's button.
 * @return padman's result, 1 once the request started, or zero when the call failed.
 */
int scePadSetMainMode(int nPort, int nSlot, int nOffset, int nLock);

/**
 * Send actuator values straight to the controller.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @param pData Six actuator bytes.
 * @return 1, or zero when the tables are not valid.
 */
int scePadSetActDirect(int nPort, int nSlot, const unsigned char *pData);

/**
 * Assign each of the six actuator bytes an actuator.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @param pData Six actuator indices, 0xff for none.
 * @return padman's result, 1 once the request started, or zero when the call failed.
 */
int scePadSetActAlign(int nPort, int nSlot, const unsigned char *pData);

/**
 * Report which buttons are pressure-sensitive.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @return The button mask, or zero when the port is closed or the controller has none.
 */
int scePadGetButtonMask(int nPort, int nSlot);

/**
 * Request the pressure-sensitive buttons.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @param nMask Buttons to report as pressures.
 * @return padman's result, 1 once the request started, or zero when the call failed.
 */
int scePadSetButtonInfo(int nPort, int nSlot, int nMask);

/**
 * Report whether a controller can report pressures.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @return 1 when every button is pressure-sensitive, or zero.
 */
int scePadInfoPressMode(int nPort, int nSlot);

/**
 * Request pressure reports for every button.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @return padman's result, 1 once the request started, or zero when the call failed.
 */
int scePadEnterPressMode(int nPort, int nSlot);

/**
 * Request digital reports for every button.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @return padman's result, 1 once the request started, or zero when the call failed.
 */
int scePadExitPressMode(int nPort, int nSlot);

/**
 * Set the pressure thresholds of a controller.
 *
 * @param nPort Port.
 * @param nSlot Slot.
 * @param pParam Twelve parameter bytes.
 * @return padman's result, 1 once the request started, or zero when the call failed.
 */
int scePadSetVrefParam(int nPort, int nSlot, const unsigned char *pParam);

/**
 * Report the number of ports.
 *
 * @return The port count, or zero when the call failed.
 */
int scePadGetPortMax(void);

/**
 * Report the number of slots of a port.
 *
 * @param nPort Port.
 * @return The slot count, or zero when the call failed.
 */
int scePadGetSlotMax(int nPort);

/**
 * Report the padman version.
 *
 * @return The major version in bits 8 to 15 and the minor in bits 0 to 7, or zero when the call
 * failed.
 */
int scePadGetModVersion(void);

/**
 * Set how much padman reports on its console.
 *
 * The name is inferred.
 *
 * @param nLevel Level.
 * @return padman's result, or zero when the call failed.
 */
int scePadSetWarningLevel(int nLevel);

#ifdef __cplusplus
}
#endif

#endif
