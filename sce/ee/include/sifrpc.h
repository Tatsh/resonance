#ifndef SIFRPC_H
#define SIFRPC_H

#include <sifcmd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Remote procedure calls between the Emotion Engine and the IOP. A client binds to a server on
 * the IOP and calls its functions. Servers on the Emotion Engine wait on a queue that a thread
 * drains with sceSifRpcLoop().
 */

/** Call mode bit that returns at once and reports completion through sceSifCheckStatRpc(). */
#define SIF_RPCM_NOWAIT 0x01

/** Call mode bit that skips the data cache write-back of the send and receive buffers. */
#define SIF_RPCM_NOWBDC 0x02

/** Command code of a finished request, sent to the side that made it. */
#define SIF_CMDC_RPC_END 0x80000008
/** Command code of a bind request. */
#define SIF_CMDC_RPC_BIND 0x80000009
/** Command code of a call request. */
#define SIF_CMDC_RPC_CALL 0x8000000a
/** Command code of a request for data from the other side's memory. */
#define SIF_CMDC_RPC_RDATA 0x8000000c

/** The request record every client and receive record begins with. */
typedef struct _sif_rpc_data {
    void *paddr;       /*!< Packet of the request in flight, null once it has finished. */
    unsigned int pid;  /*!< Identifier of the packet. */
    int tid;           /*!< Semaphore the caller waits on, or -1 for a call that does not wait. */
    unsigned int mode; /*!< Call mode bits. */
} sceSifRpcData;

/** A completion callback. It receives the argument given to sceSifCallRpc(). */
typedef void (*sceSifEndFunc)(void *para);

/**
 * A server function. It receives the function number, the argument buffer, and the argument size,
 * and returns the reply buffer, or null for no reply.
 */
typedef void *(*sceSifRpcFunc)(unsigned int fno, void *buff, int size);

struct _sif_serve_data;

/** A client bound to one server on the other side. */
typedef struct _sif_client_data {
    sceSifRpcData rpcd;            /*!< Request record. */
    unsigned int command;          /*!< Server identifier. The bind clears it. */
    void *buff;                    /*!< Server argument buffer on the other side. */
    void *cbuff;                   /*!< Server cancel buffer on the other side. */
    sceSifEndFunc func;            /*!< Completion callback of the call in flight. */
    void *para;                    /*!< Argument of the completion callback. */
    struct _sif_serve_data *serve; /*!< Server record on the other side, null until bound. */
} sceSifClientData;

/** A request for data from IOP memory made with sceSifGetOtherData(). */
typedef struct _sif_receive_data {
    sceSifRpcData rpcd; /*!< Request record. */
    void *src;          /*!< Source in IOP memory. */
    void *dest;         /*!< Destination in main memory. */
    int size;           /*!< Size in bytes. */
} sceSifReceiveData;

struct _sif_queue_data;

/** A server on the Emotion Engine. */
typedef struct _sif_serve_data {
    unsigned int command;         /*!< Server identifier. */
    sceSifRpcFunc func;           /*!< Request function. */
    void *buff;                   /*!< Argument buffer the IOP writes. */
    int size;                     /*!< Argument size of the request being served. */
    sceSifRpcFunc cfunc;          /*!< Cancel function. */
    void *cbuff;                  /*!< Cancel buffer. */
    int csize;                    /*!< Cancel argument size. */
    sceSifClientData *client;     /*!< Client record on the IOP of the request being served. */
    void *paddr;                  /*!< IOP packet of the request. */
    unsigned int fno;             /*!< Function number of the request. */
    void *receive;                /*!< IOP reply buffer of the request. */
    int rsize;                    /*!< Reply size of the request. */
    int rmode;                    /*!< Nonzero when the IOP expects a completion command. */
    unsigned int rid;             /*!< Packet record bits of the request. */
    struct _sif_serve_data *link; /*!< Next server of the same queue. */
    struct _sif_serve_data *next; /*!< Next server with a pending request. */
    struct _sif_queue_data *base; /*!< Queue the server belongs to. */
} sceSifServeData;

/** A queue of Emotion Engine servers that one thread serves. */
typedef struct _sif_queue_data {
    int key;                       /*!< Thread woken for a new request, or negative for none. */
    int active;                    /*!< Nonzero while the thread is serving a request. */
    struct _sif_serve_data *link;  /*!< First server of the queue. */
    struct _sif_serve_data *start; /*!< First server with a pending request. */
    struct _sif_serve_data *end;   /*!< Last server with a pending request. */
    struct _sif_queue_data *next;  /*!< Next queue. */
} sceSifQueueData;

/**
 * Initialise the RPC layer, and the command layer beneath it. A second call does nothing.
 *
 * @param mode Unused.
 * @ghidraAddress NTSC-U/C: 0x00564a88
 * @ghidraAddress PAL: 0x005a31f8
 */
void sceSifInitRpc(unsigned int mode);

/**
 * Stop the RPC layer and the command layer.
 *
 * @ghidraAddress NTSC-U/C: 0x00564c28
 * @ghidraAddress PAL: 0x005a3398
 */
void sceSifExitRpc(void);

/**
 * Copy data from IOP memory to main memory.
 *
 * @param rd Request record.
 * @param src Source in IOP memory.
 * @param dest Destination in main memory.
 * @param size Size in bytes.
 * @param mode Call mode bits.
 * @return Zero, -1 when no packet is free, -2 when the request could not be sent, or -3 when no
 *     semaphore could be created.
 * @ghidraAddress NTSC-U/C: 0x00564ea0
 * @ghidraAddress PAL: 0x005a3610
 */
int sceSifGetOtherData(sceSifReceiveData *rd, void *src, void *dest, int size, unsigned int mode);

/**
 * Bind a client to an IOP server. The bind succeeds before the server exists, and
 * @ref sceSifClientData::serve is then null.
 *
 * @param bd Client record to fill.
 * @param command Server identifier.
 * @param mode Call mode bits.
 * @return Zero, -1 when no packet is free, -2 when the request could not be sent, or -3 when no
 *     semaphore could be created.
 * @ghidraAddress NTSC-U/C: 0x005650f8
 * @ghidraAddress PAL: 0x005a3868
 */
int sceSifBindRpc(sceSifClientData *bd, unsigned int command, unsigned int mode);

/**
 * Call a function of a bound IOP server.
 *
 * @param bd Bound client.
 * @param fno Function number.
 * @param mode Call mode bits.
 * @param send Argument buffer.
 * @param ssize Argument size in bytes.
 * @param receive Reply buffer.
 * @param rsize Reply size in bytes.
 * @param end Completion callback, or null.
 * @param endpara Argument of the completion callback.
 * @return Zero, -1 when no packet is free, -2 when the request could not be sent, or -3 when no
 *     semaphore could be created.
 * @ghidraAddress NTSC-U/C: 0x005652c8
 * @ghidraAddress PAL: 0x005a3a38
 */
int sceSifCallRpc(sceSifClientData *bd,
                  unsigned int fno,
                  unsigned int mode,
                  void *send,
                  int ssize,
                  void *receive,
                  int rsize,
                  sceSifEndFunc end,
                  void *endpara);

/**
 * Report whether a request made with #SIF_RPCM_NOWAIT is still in flight.
 *
 * @param cd Request record of the request.
 * @return 1 while the request is in flight, otherwise 0.
 * @ghidraAddress NTSC-U/C: 0x005654b8
 * @ghidraAddress PAL: 0x005a3c28
 */
int sceSifCheckStatRpc(sceSifRpcData *cd);

/**
 * Initialise a server queue and add it to the queues bind requests search.
 *
 * @param qd Queue.
 * @param key Thread to wake for a new request, or negative for none.
 * @ghidraAddress NTSC-U/C: 0x005654f8
 * @ghidraAddress PAL: 0x005a3c68
 */
void sceSifSetRpcQueue(sceSifQueueData *qd, int key);

/**
 * Register a server on a queue.
 *
 * @param sd Server record.
 * @param command Server identifier.
 * @param func Request function.
 * @param buff Argument buffer the IOP writes.
 * @param cfunc Cancel function.
 * @param cbuff Cancel buffer.
 * @param qd Queue.
 * @ghidraAddress NTSC-U/C: 0x00565590
 * @ghidraAddress PAL: 0x005a3d00
 */
void sceSifRegisterRpc(sceSifServeData *sd,
                       unsigned int command,
                       sceSifRpcFunc func,
                       void *buff,
                       sceSifRpcFunc cfunc,
                       void *cbuff,
                       sceSifQueueData *qd);

/**
 * Remove a server from its queue.
 *
 * @param sd Server record.
 * @param qd Queue.
 * @return The server before @p sd in the queue, @p sd when it was first, or null when it was not
 *     found.
 * @ghidraAddress NTSC-U/C: 0x00565660
 * @ghidraAddress PAL: 0x005a3dd0
 */
sceSifServeData *sceSifRemoveRpc(sceSifServeData *sd, sceSifQueueData *qd);

/**
 * Remove a queue from the queues bind requests search.
 *
 * @param qd Queue.
 * @return The queue before @p qd, @p qd when it was first, or null when it was not found.
 * @ghidraAddress NTSC-U/C: 0x005656f8
 * @ghidraAddress PAL: 0x005a3e68
 */
sceSifQueueData *sceSifRemoveRpcQueue(sceSifQueueData *qd);

/**
 * Take the next server with a pending request from a queue.
 *
 * @param qd Queue.
 * @return The server, or null when no request is pending.
 * @ghidraAddress NTSC-U/C: 0x00565788
 * @ghidraAddress PAL: 0x005a3ef8
 */
sceSifServeData *sceSifGetNextRequest(sceSifQueueData *qd);

/**
 * Run the request function of a server and send its reply to the IOP.
 *
 * A request that expects no completion command has its reply packet written straight into the IOP
 * packet, retried until the DMA queue takes it.
 *
 * @param sd Server with a pending request.
 * @ghidraAddress NTSC-U/C: 0x005657e0
 * @ghidraAddress PAL: 0x005a3f50
 */
void sceSifExecRequest(sceSifServeData *sd);

/**
 * Serve the requests of a queue for ever, sleeping while no request is pending.
 *
 * @param qd Queue.
 * @ghidraAddress NTSC-U/C: 0x005659a8
 * @ghidraAddress PAL: 0x005a4118
 */
void sceSifRpcLoop(sceSifQueueData *qd);

#ifdef __cplusplus
}
#endif

#endif
