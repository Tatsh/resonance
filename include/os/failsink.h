#pragma once

class HxStr;
namespace Rnd {
class Stream;
} // namespace Rnd

/** Receives one fully formatted failure message. */
typedef void (*FailReportProc)(const char *pszMessage);

/** Finishes a failure report. The installed handler is not expected to return. */
typedef void (*FailAbortProc)();

/**
 * Sink that the engine formats its diagnostic and failure text into.
 *
 * This class is not polymorphic and has no RTTI. Its name is inferred from its two handlers and
 * from the call sites in `Rnd::Manager::Read` and `GfxDevice::InitDisplayMode`.
 *
 * Only Report() reaches the outside world in the shipped build. Print() returns without doing
 * anything, and Format() writes into a static buffer that nothing then reads. The roughly four
 * hundred call sites of each are the object dump routines across the whole engine, every
 * `DumpText` among them, so a reconstructed `DumpText` produces no output on this target. The
 * string literals it passes survive in `.rodata` regardless, which is where many recovered member
 * names come from. Reconstruct those calls as written rather than omitting them.
 *
 * The sink can also own a log file, opened by OpenLog(). No routine in the shipped image calls
 * OpenLog(), and neither Report() nor Format() writes to the log.
 */
class FailSink {
public:
    /**
     * Install the built-in report handler with no log open and a dump level of zero.
     *
     * mUnknown08 starts at 1, and mAbortProc is not written. The body is inlined into the unit's
     * static initialiser at `0x004ddef8`.
     */
    FailSink();

    /**
     * Delete the log stream, without flushing it first.
     *
     * The body is inlined into the static initialiser at `0x004ddef8`.
     */
    ~FailSink();

    /**
     * Replace the log with a file opened for writing.
     *
     * The previous log is closed first. A file that fails to open is reported as
     * "Couldn't open log %s" and leaves no log open. No call site survives in the shipped
     * program, and the name is inferred from the report text.
     *
     * @param path The file to write the log to.
     * @ghidraAddress 0x004ddfb8
     */
    void OpenLog(const HxStr &path);

    /**
     * Flush and delete the log stream, if one is open.
     *
     * OpenLog() is the one caller. The name is inferred.
     *
     * @ghidraAddress 0x004de0a0
     */
    void CloseLog();

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
     * This is the only member that produces output.
     *
     * @param pszFormat A printf-style format string.
     * @ghidraAddress 0x004dde28
     */
    void Report(const char *pszFormat, ...);

    /**
     * Format text into the secondary buffer without dispatching it.
     *
     * @param pszFormat A printf-style format string.
     * @return This sink, allowing calls to be chained.
     * @ghidraAddress 0x004dde98
     */
    FailSink *Format(const char *pszFormat, ...);

    /**
     * Write literal text.
     *
     * The body is two instructions that return the sink, so the text is discarded.
     *
     * @param pszText The text to write.
     * @return This sink, allowing calls to be chained.
     * @ghidraAddress 0x004ddfb0
     */
    FailSink *Print(const char *pszText);

    /**
     * Handler that finishes a failure report.
     *
     * Public because `main` assigns it directly and the image has no accessor for it, unlike the
     * report handler.
     *
     * +0x00
     */
    FailAbortProc mAbortProc;

private:
    FailReportProc mReportProc; // +0x04
    int mUnknown08;             // +0x08

public:
    /**
     * Detail level a dump routine tests before writing its optional sections.
     *
     * Public because `Rnd::Object::DumpText` reads it directly from a sink it does not own, at
     * `0x0053e6d4`, and the image exposes no accessor. A positive value adds the referrer list.
     *
     * +0x0c
     */
    int mDumpLevel;

private:
    Rnd::Stream *mLogStream; // +0x10
};

/**
 * Engine-wide failure sink.
 *
 * @ghidraAddress 0x00702470
 */
extern FailSink g_failSink;
