#ifndef SIFRPC_H
#define SIFRPC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SIF remote procedure call services of the resident sifcmd library. The layouts follow the stabs
 * the shipped EZMIDI.IRX records.
 */

/** A server function. It receives the function number, the argument buffer, and its size. */
typedef void *(*sceSifRpcFunc)(unsigned int fno, void *buff, int size);

/** The request header every RPC packet begins with. */
typedef struct _sif_rpc_data {
    void *paddr;       /*!< Packet address on the sending side. */
    unsigned int pid;  /*!< Packet identifier. */
    int tid;           /*!< Thread waiting on the packet. */
    unsigned int mode; /*!< Call mode bits. */
} sceSifRpcData;

struct _sif_client_data;
struct _sif_queue_data;

/** A registered server. */
typedef struct _sif_serve_data {
    unsigned int command;             /*!< Server identifier. */
    sceSifRpcFunc func;               /*!< Request handler. */
    void *buff;                       /*!< Argument buffer. */
    int size;                         /*!< Argument buffer size. */
    sceSifRpcFunc cfunc;              /*!< Cancel handler. */
    void *cbuff;                      /*!< Cancel buffer. */
    int csize;                        /*!< Cancel buffer size. */
    struct _sif_client_data *client;  /*!< Client of the current request. */
    void *paddr;                      /*!< Packet address of the current request. */
    unsigned int fno;                 /*!< Function number of the current request. */
    void *receive;                    /*!< Reply buffer on the client side. */
    int rsize;                        /*!< Reply size. */
    int rmode;                        /*!< Reply mode. */
    unsigned int rid;                 /*!< Reply identifier. */
    struct _sif_serve_data *link;     /*!< Next server of the queue. */
    struct _sif_serve_data *next;     /*!< Next pending request. */
    struct _sif_queue_data *base;     /*!< Queue the server belongs to. */
} sceSifServeData;

/** A request queue one server thread drains. */
typedef struct _sif_queue_data {
    int key;                        /*!< Thread that drains the queue. */
    int active;                     /*!< Nonzero while a request is being served. */
    struct _sif_serve_data *link;   /*!< First registered server. */
    struct _sif_serve_data *start;  /*!< First pending request. */
    struct _sif_serve_data *end;    /*!< Last pending request. */
    struct _sif_queue_data *next;   /*!< Next queue of the system. */
} sceSifQueueData;

/**
 * Initialise the RPC layer.
 *
 * @param mode Reserved, zero.
 */
void sceSifInitRpc(unsigned int mode);

/**
 * Bind a queue to a server thread.
 *
 * @param qd Queue to initialise.
 * @param key Thread identifier that drains the queue.
 */
void sceSifSetRpcQueue(sceSifQueueData *qd, int key);

/**
 * Register a server on a queue.
 *
 * @param sd Server record to fill.
 * @param command Server identifier clients bind to.
 * @param func Request handler.
 * @param buff Argument buffer.
 * @param cfunc Cancel handler, or null.
 * @param cbuff Cancel buffer, or null.
 * @param qd Queue the server joins.
 * @return @p sd.
 */
sceSifServeData *sceSifRegisterRpc(sceSifServeData *sd,
                                   unsigned int command,
                                   sceSifRpcFunc func,
                                   void *buff,
                                   sceSifRpcFunc cfunc,
                                   void *cbuff,
                                   sceSifQueueData *qd);

/**
 * Serve requests on a queue forever.
 *
 * @param qd Queue to drain.
 */
void sceSifRpcLoop(sceSifQueueData *qd);

#ifdef __cplusplus
}
#endif

#endif
