#include <deci2.h>
#include <eekernel.h>
#include <eeregs.h>
#include <libkernelinternal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

enum {
    kStatusEie = 0x10000,
    kT3ModeCompareInterrupt = 0x100,
    kSioIsrTxFull = 0x8000,

    kSyscallAlarmReturn = 8,
    kSyscallKernelCopy = 90,
    kSyscallGetEntryAddress = 91,
    kSyscallSetAlarm = 252,
    kSyscallReleaseAlarm = 253,
    kSyscallISetAlarm = 254,
    kSyscallIReleaseAlarm = 255,
    kSyscallAlarmInterrupt = 300,
    kSyscallTableSize = 8,
    kSyscallTableFirstLookup = 2,

    kDCacheLineSize = 64,
    kDCacheIndexEnd = 4096,
    kDCacheTagAddressMask = 0xfffff000,

    kThreadRequestWakeup = 0,
    kThreadRequestRotate = 1,
    kThreadRequestSuspend = 2,
    kThreadRequestQueueSize = 512,
    kThreadRequestIndexMask = kThreadRequestQueueSize - 1,
    kThreadRequestSemaMax = 255,
    kHelperStackSize = 1024,
    kHelperPriority = 0,
    kCallerPriority = 1,
    kThreadIdLimit = 256,

    kDeci2ArgCount = 4,
    kDeci2WorkAreaSize = 40,
    kDeci2Open = 1,
    kDeci2ReqSend = 3,
    kDeci2Poll = 4,
    kDeci2ExRecv = -5,
    kDeci2ExSend = -6,
    kDeci2Kputs = 16,

    kConsoleLineSize = 128,
    kConsoleLineFlush = 126,
    kPrintfBufferSize = 32,
    kPrintfMaxWidth = 31,
    kDecimalBase = 10,
    kOctalShift = 3,
    kOctalMask = 7,
    kHexShift = 4,
    kHexMask = 15,

    kDoubleSignAndExponentBits = 12,
    kDoubleExponentShift = 53,
    kDoubleMantissaBits = 52,
    kDoubleExponentBias = 1075,
    kDoubleExponentMin = -53,
    kDoubleExponentMax = 13,
    kDoubleOverflowValue = 9999,
    kRoundBits = 2,
    kRoundMask = 3,
    kShiftCountMask = 63,
};

// The kernel addresses the alarm patch and its handler trampoline are copied to.
#define kAlarmPatchBase ((void *)0x80076000)
#define kAlarmHandlerTrampoline ((void *)0x00082000)

// One entry of the table InstallSyscallPatch() works through.
typedef struct {
    int nSyscall;
    void *pAddress;
} SyscallEntry;

// A thread request an interrupt handler passes to the helper thread.
typedef struct {
    unsigned char nType;
    unsigned char nId;
} ThreadRequest;

// The requests the helper thread performs, in arrival order.
typedef struct {
    int nHead;
    int nTail;
    ThreadRequest mRequests[kThreadRequestQueueSize];
} ThreadRequestQueue;

typedef void (*PutCharFunction)(int c);

// The fraction printfloat() prints has six digits.
static const double kFractionScale = 1000000.0;

// NTSC-U/C: 0x00769820, PAL: 0x007ad578
static SyscallEntry g_syscallPatchTable[kSyscallTableSize] = {
    {kSyscallKernelCopy, KernelCopyHandler},
    {kSyscallGetEntryAddress, kAlarmPatchBase},
    {kSyscallSetAlarm, NULL},
    {kSyscallISetAlarm, NULL},
    {kSyscallReleaseAlarm, NULL},
    {kSyscallIReleaseAlarm, NULL},
    {kSyscallAlarmInterrupt, NULL},
    {kSyscallAlarmReturn, NULL},
};

// NTSC-U/C: 0x006fc33c, PAL: 0x0073fd1c
static int g_libcSemaphores[2];

// NTSC-U/C: 0x0077f988, PAL: 0x007c3778
static int topId;

// NTSC-U/C: 0x008e5350, PAL: 0x0092a350
static unsigned char g_helperStack[kHelperStackSize] __attribute__((aligned(16)));

// NTSC-U/C: 0x008e5750, PAL: 0x0092a750
static int topSema;

// NTSC-U/C: 0x008e5758, PAL: 0x0092a758
static ThreadRequestQueue topArg;

// NTSC-U/C: 0x008e6950, PAL: 0x0092b950
static unsigned char g_deci2WorkArea[kDeci2WorkAreaSize] __attribute__((aligned(16)));

static inline int InterruptsEnabled(void) {
    unsigned int status;

    __asm__ volatile("mfc0 %0, $12" : "=r"(status));
    return (status & kStatusEie) != 0;
}

static inline void SyncL(void) {
    __asm__ volatile("sync.l" ::: "memory");
}

// System call -47. The binary makes the call inline.
static inline int iGetThreadId(void) {
    register int result __asm__("v0");

    __asm__ volatile("addiu $v1, $zero, -47\n\tsyscall" : "=r"(result) : : "v1", "memory");
    return result;
}

int DIntr(void) {
    if (!InterruptsEnabled()) {
        return 0;
    }
    // A di on this processor can fail to take effect. The loop retries it until EIE clears.
    do {
        __asm__ volatile("di\n\tsync.p" ::: "memory");
    } while (InterruptsEnabled());
    return 1;
}

int EIntr(void) {
    int bWasEnabled = InterruptsEnabled();

    __asm__ volatile("ei" ::: "memory");
    return bWasEnabled;
}

int DisableIntc(int cause) {
    int bEnabled = InterruptsEnabled();
    int result;

    if (bEnabled) {
        DIntr();
    }
    result = _DisableIntc(cause);
    SyncL();
    if (bEnabled) {
        EIntr();
    }
    return result;
}

int EnableIntc(int cause) {
    int bEnabled = InterruptsEnabled();
    int result;

    if (bEnabled) {
        DIntr();
    }
    result = _EnableIntc(cause);
    SyncL();
    if (bEnabled) {
        EIntr();
    }
    return result;
}

int DisableDmac(int channel) {
    int bEnabled = InterruptsEnabled();
    int result;

    if (bEnabled) {
        DIntr();
    }
    result = _DisableDmac(channel);
    SyncL();
    if (bEnabled) {
        EIntr();
    }
    return result;
}

int EnableDmac(int channel) {
    int bEnabled = InterruptsEnabled();
    int result;

    if (bEnabled) {
        DIntr();
    }
    result = _EnableDmac(channel);
    SyncL();
    if (bEnabled) {
        EIntr();
    }
    return result;
}

int KernelCopyHandler(unsigned int *dest, const unsigned int *src, unsigned int size) {
    unsigned int nWords = size >> 2;
    unsigned int i;

    for (i = 0; i < nWords; ++i) {
        *dest++ = *src++;
    }
    return 0;
}

void InstallSyscallPatch(void) {
    int i;

    if ((*T3_MODE & kT3ModeCompareInterrupt) != 0) {
        return;
    }
    SetSyscall(g_syscallPatchTable[0].nSyscall, g_syscallPatchTable[0].pAddress);
    KernelCopy(kAlarmPatchBase, g_alarmPatch, kAlarmPatchSize);
    KernelCopy(kAlarmHandlerTrampoline, g_alarmTrampoline, kAlarmTrampolineSize);
    FlushCache(WRITEBACK_DCACHE);
    FlushCache(INVALIDATE_ICACHE);
    SetSyscall(g_syscallPatchTable[1].nSyscall, g_syscallPatchTable[1].pAddress);
    for (i = kSyscallTableFirstLookup; i < kSyscallTableSize; ++i) {
        SetSyscall(g_syscallPatchTable[i].nSyscall,
                   GetEntryAddress(g_syscallPatchTable[i].nSyscall));
    }
}

// NTSC-U/C: 0x004b8f98, PAL: 0x004f6f10
static void InitLibcSemas(void) {
    struct SemaParam first;
    struct SemaParam second;

    first.maxCount = 1;
    first.initCount = 1;
    second.maxCount = 1;
    second.initCount = 1;
    g_libcSemaphores[0] = CreateSema(&first);
    g_libcSemaphores[1] = CreateSema(&second);
}

void _InitSys(void) {
    InitLibcSemas();
    InstallSyscallPatch();
    (void)InitThread();
}

// NTSC-U/C: 0x005f1fb0, PAL: 0x006340f8
static void topThread(void *arg) {
    ThreadRequestQueue *pQueue = (ThreadRequestQueue *)arg;

    for (;;) {
        int nIndex;
        const ThreadRequest *pRequest;

        WaitSema(topSema);
        nIndex = pQueue->nHead & kThreadRequestIndexMask;
        pQueue->nHead = nIndex + 1;
        pRequest = &pQueue->mRequests[nIndex];
        switch (pRequest->nType) {
        case kThreadRequestWakeup:
            WakeupThread(pRequest->nId);
            break;
        case kThreadRequestRotate:
            RotateThreadReadyQueue(pRequest->nId);
            break;
        case kThreadRequestSuspend:
            SuspendThread(pRequest->nId);
            break;
        default:
            kprintf("## internel error in libkernl.a!\n");
            break;
        }
    }
}

int InitThread(void) {
    struct ThreadParam thread;
    struct SemaParam sema;

    if (topId > 0) {
        return -1;
    }
    sema.initCount = 0;
    sema.maxCount = kThreadRequestSemaMax;
    topSema = CreateSema(&sema);
    if (topSema < 0) {
        return -1;
    }
    thread.entry = topThread;
    thread.stack = g_helperStack;
    thread.stackSize = kHelperStackSize;
    thread.gpReg = _gp;
    thread.initPriority = kHelperPriority;
    topId = CreateThread(&thread);
    if (topId < 0) {
        DeleteSema(topSema);
        return -1;
    }
    topArg.nHead = 0;
    topArg.nTail = 0;
    StartThread(topId, &topArg);
    ChangeThreadPriority(GetThreadId(), kCallerPriority);
    return topId;
}

int iWakeupThread(int thid) {
    int nSelf = iGetThreadId();
    int nIndex;

    if (nSelf != thid) {
        return _iWakeupThread(thid);
    }
    if ((unsigned int)nSelf >= kThreadIdLimit || topId == 0) {
        return -1;
    }
    nIndex = topArg.nTail & kThreadRequestIndexMask;
    topArg.nTail = nIndex + 1;
    topArg.mRequests[nIndex].nType = kThreadRequestWakeup;
    topArg.mRequests[nIndex].nId = (unsigned char)nSelf;
    iSignalSema(topSema);
    return nSelf;
}

// NTSC-U/C: 0x00620730, PAL: 0x006612c0
// Writes back every data cache line whose tag lies in the range. Cache operation 0x10 loads the tag
// of a line into TagLo, and 0x14 writes the line back and invalidates it. Bit 0 of the index
// selects the way.
static void _sceSDC(uintptr_t start, uintptr_t end) {
    unsigned int nIndex;

    for (nIndex = 0; nIndex < kDCacheIndexEnd; nIndex += kDCacheLineSize) {
        uintptr_t line;
        unsigned int tag;

        __asm__ volatile("sync.l\n\tcache 0x10, 0(%1)\n\tsync.l\n\tmfc0 %0, $28"
                         : "=r"(tag)
                         : "r"(nIndex)
                         : "memory");
        line = (tag & kDCacheTagAddressMask) + nIndex;
        if (!(line < start) && !(end < line)) {
            __asm__ volatile("sync.l\n\tcache 0x14, 0(%0)\n\tsync.l" : : "r"(nIndex) : "memory");
        }
        __asm__ volatile("sync.l\n\tcache 0x10, 1(%1)\n\tsync.l\n\tmfc0 %0, $28"
                         : "=r"(tag)
                         : "r"(nIndex)
                         : "memory");
        line = (tag & kDCacheTagAddressMask) + nIndex;
        if (!(line < start) && !(end < line)) {
            __asm__ volatile("sync.l\n\tcache 0x14, 1(%0)\n\tsync.l" : : "r"(nIndex) : "memory");
        }
        SyncL();
    }
}

void SyncDCache(void *start, void *end) {
    int bEnabled = InterruptsEnabled();

    if (bEnabled) {
        DIntr();
    }
    _sceSDC((uintptr_t)start & ~(uintptr_t)(kDCacheLineSize - 1),
            (uintptr_t)end & ~(uintptr_t)(kDCacheLineSize - 1));
    if (bEnabled) {
        EIntr();
    }
}

int sceDeci2Open(unsigned short protocol,
                 void *opt,
                 void (*handler)(int event, int param, void *opt)) {
    unsigned int args[kDeci2ArgCount];

    args[0] = protocol;
    args[1] = (uintptr_t)opt;
    args[2] = (uintptr_t)handler;
    args[3] = (uintptr_t)UNCACHED_SEG(g_deci2WorkArea);
    return Deci2Call(kDeci2Open, args);
}

int sceDeci2ReqSend(int s, char dest) {
    unsigned int args[kDeci2ArgCount];

    args[0] = s;
    args[1] = (int)(signed char)dest;
    return Deci2Call(kDeci2ReqSend, args);
}

int sceDeci2Poll(int s) {
    unsigned int args[kDeci2ArgCount];

    args[0] = s;
    return Deci2Call(kDeci2Poll, args);
}

int sceDeci2ExRecv(int s, void *buf, unsigned short len) {
    unsigned int args[kDeci2ArgCount];

    args[0] = s;
    args[1] = (uintptr_t)buf;
    args[2] = len;
    return Deci2Call(kDeci2ExRecv, args);
}

int sceDeci2ExSend(int s, void *buf, unsigned short len) {
    unsigned int args[kDeci2ArgCount];

    args[0] = s;
    args[1] = (uintptr_t)buf;
    args[2] = len;
    return Deci2Call(kDeci2ExSend, args);
}

int kputs(char *s) {
    unsigned int args[kDeci2ArgCount];

    args[0] = (uintptr_t)s;
    return Deci2Call(kDeci2Kputs, args);
}

// NTSC-U/C: 0x008e5bf8, PAL: 0x0092abf8
static char linebuf[kConsoleLineSize];

// NTSC-U/C: 0x00780dc0, PAL: 0x007c4ad8
static int count;

int kputchar(int c) {
    while ((*SIO_ISR & kSioIsrTxFull) != 0) {
    }
    *SIO_TXFIFO = (unsigned char)c;
    return c;
}

// NTSC-U/C: 0x005fa8f8, PAL: 0x0063b608
// Collects a line for the DECI2 kernel console. A long line is sent in parts.
static void PutConsoleLineChar(int c) {
    int nLength;

    if (count >= kConsoleLineFlush) {
        count = 0;
        linebuf[kConsoleLineSize - 1] = '\0';
        kputs(linebuf);
    }
    nLength = count;
    if (c == '\n') {
        count = 0;
        linebuf[nLength] = (char)c;
        linebuf[nLength + 1] = '\0';
        kputs(linebuf);
        return;
    }
    count = nLength + 1;
    linebuf[nLength] = (char)c;
}

// NTSC-U/C: 0x005fa9a8, PAL: 0x0063b6b8
static void serialPutchar(int c) {
    if (c == '\n') {
        kputchar('\r');
        kputchar('\n');
        return;
    }
    kputchar(c);
}

// NTSC-U/C: 0x00780dc4, PAL: 0x007c4adc
static PutCharFunction _putchar = serialPutchar;

// NTSC-U/C: 0x005fa9e0, PAL: 0x0063b6f0
// Converts the bits of a double to an integer. A fraction of three quarters or more rounds up, and
// a value of 2^13 or more returns 9999.
static int ftoi(unsigned long long bits) {
    int nExponent = (int)((bits << 1) >> kDoubleExponentShift) - kDoubleExponentBias;
    unsigned long long mantissa;

    if (nExponent < kDoubleExponentMin) {
        return 0;
    }
    if (nExponent >= kDoubleExponentMax) {
        return kDoubleOverflowValue;
    }
    mantissa = ((bits << kDoubleSignAndExponentBits) >> kDoubleSignAndExponentBits) |
               (1ULL << kDoubleMantissaBits);
    if (nExponent < 0) {
        // The hardware masks the count. An exponent of -1 shifts by 63.
        mantissa >>= (-nExponent - kRoundBits) & kShiftCountMask;
        if ((mantissa & kRoundMask) == kRoundMask) {
            mantissa = (mantissa >> kRoundBits) + 1;
        } else {
            mantissa >>= kRoundBits;
        }
    } else {
        mantissa <<= nExponent;
    }
    return (int)mantissa;
}

// NTSC-U/C: 0x005faa70, PAL: 0x0063b780
// Prints a value as 0.digits and a power of ten.
static void printfloat(double value) {
    int nExponent = 0;

    if (value < 0.0) {
        value = 0.0 - value;
        _putchar('-');
    }
    if (value < 0.1) {
        while (value < 0.1) {
            value *= 10.0;
            --nExponent;
        }
    } else {
        // __cmpdf2 orders NaN above every value. The binary never ends this loop for NaN.
        while (!(value < 1.0)) {
            value /= 10.0;
            ++nExponent;
        }
    }
    // Yes, the binary passes the converted integer where ftoi() expects double bits.
    kprintf("0.%d", ftoi((unsigned long long)(value * kFractionScale)));
    if (nExponent >= 0) {
        kprintf("e+%d", nExponent);
    } else {
        kprintf("e%d", nExponent);
    }
}

static void PutString(const char *psz) {
    while (*psz != '\0') {
        _putchar((signed char)*psz++);
    }
}

// Returns the start of the digits, or of the zero padding when the padding begins earlier.
static const char *ApplyPadding(const char *pDigits, const char *pPad) {
    if (pPad != NULL && pPad < pDigits) {
        return pPad;
    }
    return pDigits;
}

// NTSC-U/C: 0x005fabd8, PAL: 0x0063b8e8
static void _printf(const char *pszFormat, va_list args) {
    char szDigits[kPrintfBufferSize];
    const char *p = pszFormat;

    while (*p != '\0') {
        char *pPad = NULL;
        int nSize = 0;
        unsigned long long value;
        long long nSigned;
        char *pOut;

        if (*p != '%') {
            _putchar((signed char)*p++);
            continue;
        }
        ++p;
    parse:
        switch (*p) {
        case '0': {
            int nWidth = p[1] - '0';
            int i;

            if ((unsigned char)nWidth >= kDecimalBase) {
                ++p;
                goto parse;
            }
            if ((unsigned int)(p[2] - '0') < kDecimalBase) {
                nWidth = nWidth * kDecimalBase + (p[2] - '0');
                p += 2;
                if (nWidth > kPrintfMaxWidth) {
                    nWidth = kPrintfMaxWidth;
                }
            } else {
                p += 1;
            }
            pPad = &szDigits[kPrintfMaxWidth - nWidth];
            for (i = nWidth; i > 0; --i) {
                szDigits[kPrintfMaxWidth - i] = '0';
            }
            ++p;
            goto parse;
        }
        case 'l':
        case 'h':
            nSize = *p++;
            goto parse;
        case 'o':
        case 'x':
        case 'u':
            if (nSize == 'l') {
                value = va_arg(args, unsigned long long);
            } else if (nSize == 'h') {
                value = (unsigned short)va_arg(args, unsigned int);
            } else {
                value = va_arg(args, unsigned int);
            }
            pOut = &szDigits[kPrintfMaxWidth];
            *pOut = '\0';
            if (value == 0) {
                *--pOut = '0';
            } else if (*p == 'o') {
                do {
                    *--pOut = (char)('0' + (value & kOctalMask));
                    value >>= kOctalShift;
                } while (value != 0);
            } else if (*p == 'x') {
                do {
                    unsigned int nDigit = value & kHexMask;

                    *--pOut = (char)(nDigit < kDecimalBase ? '0' + nDigit : 'a' + nDigit - 10);
                    value >>= kHexShift;
                } while (value != 0);
            } else {
                do {
                    *--pOut = (char)('0' + value % kDecimalBase);
                    value /= kDecimalBase;
                } while (value != 0);
            }
            ++p;
            PutString(ApplyPadding(pOut, pPad));
            break;
        case 'd':
            if (nSize == 'l') {
                nSigned = va_arg(args, long long);
            } else if (nSize == 'h') {
                nSigned = (short)va_arg(args, int);
            } else {
                nSigned = va_arg(args, int);
            }
            pOut = &szDigits[kPrintfMaxWidth];
            *pOut = '\0';
            if (nSigned == 0) {
                *--pOut = '0';
            } else {
                // The sign goes out before the padding.
                if (nSigned < 0) {
                    nSigned = -nSigned;
                    _putchar('-');
                }
                do {
                    *--pOut = (char)('0' + nSigned % kDecimalBase);
                    nSigned /= kDecimalBase;
                } while (nSigned != 0);
            }
            ++p;
            PutString(ApplyPadding(pOut, pPad));
            break;
        case 'e':
        case 'f': {
            // The binary read a single-precision value. This compiler passes the value as a
            // double, and the value is rounded to single precision before the zero test and the
            // print.
            const float flReal = (float)va_arg(args, double);

            if (flReal == 0.0f) {
                _putchar('0');
            } else {
                printfloat(flReal);
            }
            ++p;
            break;
        }
        case 's': {
            const char *psz = va_arg(args, const char *);

            // Yes, the binary prints an empty string as (null).
            PutString(*psz == '\0' ? "(null)" : psz);
            ++p;
            break;
        }
        case 'c':
            _putchar((signed char)va_arg(args, int));
            ++p;
            break;
        default:
            // Yes, the binary also steps past a terminator that follows '%'.
            ++p;
            break;
        }
    }
}

void kprintf(const char *format, ...) {
    va_list args;

    va_start(args, format);
    _printf(format, args);
    va_end(args);
}

void scePrintf(const char *format, ...) {
    PutCharFunction pfnSaved = _putchar;
    va_list args;

    _putchar = PutConsoleLineChar;
    va_start(args, format);
    _printf(format, args);
    va_end(args);
    _putchar = pfnSaved;
}
