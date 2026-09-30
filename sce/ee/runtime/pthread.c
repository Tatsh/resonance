// The POSIX thread calls the C++ runtime makes, over the kernel's threads and semaphores. The
// original was built with a compiler whose runtime did not make these calls. No code here comes
// from the shipped executable. Only the calls the runtime makes are provided.

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include <eekernel.h>

enum {
    kMaxKeys = 32,
    kMaxThreads = 256, // Thread identifiers the kernel hands out.
    kSemaMax = 0x7fffffff,
};

struct pthread_mutex_t_ {
    int sema;
    int owner;
    int depth;
    int recursive;
};

struct pthread_cond_t_ {
    int sema;
    int waiters;
};

struct pthread_key_t_ {
    int index;
};

static struct {
    int used;
    void (*destructor)(void *);
    void **values;
} g_keys[kMaxKeys];

static struct pthread_key_t_ g_keyHandles[kMaxKeys];

static int NewSema(int initCount, int maxCount) {
    struct SemaParam param = {0};

    param.initCount = initCount;
    param.maxCount = maxCount;
    return CreateSema(&param);
}

// Replaces a static initialiser with a real object. Interrupts are disabled around the check so two
// threads cannot both allocate.
static struct pthread_mutex_t_ *MutexOf(pthread_mutex_t *mutex) {
    struct pthread_mutex_t_ *pMutex;
    const int enabled = DIntr();

    if (*mutex == PTHREAD_MUTEX_INITIALIZER || *mutex == PTHREAD_RECURSIVE_MUTEX_INITIALIZER ||
        *mutex == PTHREAD_ERRORCHECK_MUTEX_INITIALIZER) {
        pMutex = calloc(1, sizeof(*pMutex));
        pMutex->sema = NewSema(1, 1);
        pMutex->owner = -1;
        pMutex->recursive = (*mutex == PTHREAD_RECURSIVE_MUTEX_INITIALIZER);
        *mutex = pMutex;
    }
    pMutex = *mutex;
    if (enabled) {
        EIntr();
    }
    return pMutex;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    (void)attr; // The runtime never sets mutex attributes.
    *mutex = PTHREAD_MUTEX_INITIALIZER;
    MutexOf(mutex);
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    if (*mutex != PTHREAD_MUTEX_INITIALIZER && *mutex != PTHREAD_RECURSIVE_MUTEX_INITIALIZER &&
        *mutex != PTHREAD_ERRORCHECK_MUTEX_INITIALIZER) {
        DeleteSema((*mutex)->sema);
        free(*mutex);
    }
    *mutex = PTHREAD_MUTEX_INITIALIZER;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    struct pthread_mutex_t_ *pMutex = MutexOf(mutex);
    const int self = GetThreadId();

    if (pMutex->recursive && pMutex->owner == self) {
        ++pMutex->depth;
        return 0;
    }
    WaitSema(pMutex->sema);
    pMutex->owner = self;
    pMutex->depth = 1;
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    struct pthread_mutex_t_ *pMutex = MutexOf(mutex);

    if (--pMutex->depth > 0) {
        return 0;
    }
    pMutex->owner = -1;
    SignalSema(pMutex->sema);
    return 0;
}

static struct pthread_cond_t_ *CondOf(pthread_cond_t *cond) {
    struct pthread_cond_t_ *pCond;
    const int enabled = DIntr();

    if (*cond == PTHREAD_COND_INITIALIZER) {
        pCond = calloc(1, sizeof(*pCond));
        pCond->sema = NewSema(0, kSemaMax);
        *cond = pCond;
    }
    pCond = *cond;
    if (enabled) {
        EIntr();
    }
    return pCond;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
    struct pthread_cond_t_ *pCond = CondOf(cond);

    ++pCond->waiters;
    pthread_mutex_unlock(mutex);
    WaitSema(pCond->sema);
    pthread_mutex_lock(mutex);
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond) {
    struct pthread_cond_t_ *pCond = CondOf(cond);
    const int enabled = DIntr();
    const int waiters = pCond->waiters;

    pCond->waiters = 0;
    if (enabled) {
        EIntr();
    }
    for (int i = 0; i < waiters; ++i) {
        SignalSema(pCond->sema);
    }
    return 0;
}

int pthread_once(pthread_once_t *once, void (*init)(void)) {
    static pthread_mutex_t onceLock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;

    if (once->done) {
        return 0;
    }
    pthread_mutex_lock(&onceLock);
    if (!once->done) {
        init();
        once->done = 1;
    }
    pthread_mutex_unlock(&onceLock);
    return 0;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *)) {
    const int enabled = DIntr();
    int index;

    for (index = 0; index < kMaxKeys && g_keys[index].used; ++index) {
    }
    if (index < kMaxKeys) {
        g_keys[index].used = 1;
    }
    if (enabled) {
        EIntr();
    }
    if (index == kMaxKeys) {
        return EAGAIN;
    }
    g_keys[index].destructor = destructor;
    g_keys[index].values = calloc(kMaxThreads, sizeof(void *));
    g_keyHandles[index].index = index;
    *key = &g_keyHandles[index];
    return 0;
}

int pthread_key_delete(pthread_key_t key) {
    free(g_keys[key->index].values);
    g_keys[key->index].values = NULL;
    g_keys[key->index].used = 0;
    return 0;
}

void *pthread_getspecific(pthread_key_t key) {
    return g_keys[key->index].values[GetThreadId() % kMaxThreads];
}

int pthread_setspecific(pthread_key_t key, const void *value) {
    g_keys[key->index].values[GetThreadId() % kMaxThreads] = (void *)value;
    return 0;
}
