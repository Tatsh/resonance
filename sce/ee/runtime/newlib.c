// The system layer the C library calls, over the kernel and the file service. The original linked
// the SDK C library and did not need this layer. No code here comes from the shipped executable.

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/lock.h>

#include <eekernel.h>

// The C library's locks. A lock takes its semaphore on first use, because the library locks some of
// them before the kernel is ready.
struct __lock {
    int sema;
    int owner;
    int depth;
};

enum {
    kNoSema = -1,
    kNoOwner = -1,
};

#define STATIC_LOCK(name) struct __lock __lock_##name = {kNoSema, kNoOwner, 0}

STATIC_LOCK(__arc4random_mutex);
STATIC_LOCK(__atexit_recursive_mutex);
STATIC_LOCK(__at_quick_exit_mutex);
STATIC_LOCK(__dd_hash_mutex);
STATIC_LOCK(__env_recursive_mutex);
STATIC_LOCK(__malloc_recursive_mutex);
STATIC_LOCK(__sfp_recursive_mutex);
STATIC_LOCK(__tz_mutex);

extern char _end[];

static void LockAcquire(struct __lock *lock) {
    const int self = GetThreadId();

    if (lock->sema == kNoSema) {
        struct SemaParam param = {0};
        const int enabled = DIntr();

        if (lock->sema == kNoSema) {
            param.initCount = 1;
            param.maxCount = 1;
            lock->sema = CreateSema(&param);
        }
        if (enabled) {
            EIntr();
        }
    }
    if (lock->owner == self) {
        ++lock->depth;
        return;
    }
    WaitSema(lock->sema);
    lock->owner = self;
    lock->depth = 1;
}

static void LockRelease(struct __lock *lock) {
    if (--lock->depth > 0) {
        return;
    }
    lock->owner = kNoOwner;
    SignalSema(lock->sema);
}

void __retarget_lock_init_recursive(_LOCK_T *lock) {
    *lock = calloc(1, sizeof(struct __lock));
    (*lock)->sema = kNoSema;
    (*lock)->owner = kNoOwner;
}

void __retarget_lock_close_recursive(_LOCK_T lock) {
    if (lock->sema != kNoSema) {
        DeleteSema(lock->sema);
    }
    free(lock);
}

void __retarget_lock_acquire(_LOCK_T lock) {
    LockAcquire(lock);
}

void __retarget_lock_acquire_recursive(_LOCK_T lock) {
    LockAcquire(lock);
}

void __retarget_lock_release(_LOCK_T lock) {
    LockRelease(lock);
}

void __retarget_lock_release_recursive(_LOCK_T lock) {
    LockRelease(lock);
}

// The heap runs from the end of .bss to the limit the kernel reports.
void *_sbrk(ptrdiff_t increment) {
    static char *s_pBreak = _end;
    char *pOld = s_pBreak;

    if (&s_pBreak[increment] > (char *)EndOfHeap()) {
        errno = ENOMEM;
        return (void *)-1;
    }
    s_pBreak = &s_pBreak[increment];
    return pOld;
}

void _exit(int status) {
    Exit(status);
    for (;;) {
    }
}

int _getpid(void) {
    return 1;
}

int _kill(int pid, int sig) {
    (void)pid;
    (void)sig;
    errno = ENOSYS;
    return -1;
}

int _getentropy(void *buffer, size_t length) {
    (void)buffer;
    (void)length;
    errno = ENOSYS;
    return -1;
}

int _fcntl(int fd, int cmd, ...) {
    (void)fd;
    (void)cmd;
    errno = ENOSYS;
    return -1;
}

int getdents(int fd, void *buffer, int length) {
    (void)fd;
    (void)buffer;
    (void)length;
    errno = ENOSYS;
    return -1;
}
