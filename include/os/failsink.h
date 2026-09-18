#pragma once

/** Receives one fully formatted failure message. */
typedef void (*FailReportProc)(const char *pszMessage);

/** Finishes a failure report. The installed handler is not expected to return. */
typedef void (*FailAbortProc)();

/**
 * Sink that the engine formats its failure reports into.
 *
 * Engine asserts format their text through Report() and then transfer control to the abort
 * handler. The application therefore decides whether a failed check halts the machine. This class
 * is not polymorphic and has no RTTI. Its name is inferred from the two handlers and from the call
 * sites in `Rnd::Manager::Read` and `GfxDevice::InitDisplayMode`.
 */
class FailSink {
public:
    /**
     * Install the handler that receives formatted failure text.
     *
     * A null handler restores the built-in handler at 0x004ddf68.
     *
     * @param pfnReport The handler to install, or null for the built-in handler.
     * @ghidraAddress 0x004ddf90
     */
    void SetReportHandler(FailReportProc pfnReport);

    /**
     * Format a failure message and pass it to the installed report handler.
     *
     * @param pszFormat A printf-style format string.
     * @ghidraAddress 0x004dde28
     */
    void Report(const char *pszFormat, ...);

    /**
     * Format a failure message into the secondary buffer without dispatching it.
     *
     * @param pszFormat A printf-style format string.
     * @return This sink, allowing reports to be chained.
     * @ghidraAddress 0x004dde98
     */
    FailSink *Format(const char *pszFormat, ...);

    FailAbortProc mAbortProc;   // +0x00
    FailReportProc mReportProc; // +0x04
};

/**
 * Engine-wide failure sink.
 *
 * @ghidraAddress 0x00702470
 */
extern FailSink g_failSink;
