#include "netflow/netflow.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include "os/log.h"

extern "C" {

// The edge builder requests eight byte records from this allocator.
void *NetflowAllocChecked(unsigned nSize);

// Reports an empty edge pool.
void NetflowReportOutOfSpace(const char *pszMessage);

// The matcher runs this greedy pass first and reports the result.
void NetflowReportInitialMatching(struct netflow_graph *graph, struct netflow_v_side *side);

// The matcher runs this pool pass between the greedy pass and the search.
void NetflowInitMatchingPool(struct netflow_graph *graph, struct netflow_v_side *side);

// The matcher runs this augmenting path search last.
void NetflowFindAugmentingPaths(struct netflow_graph *graph, struct netflow_v_side *side);

} // extern "C"

// 0x007B88D8
// Edge examinations counted across the search, statistics only.
int g_nNetflowProbeCount = 0;

// 0x007B88DC
// Augmentations applied, statistics only.
int g_nNetflowAugmentTotal = 0;

// 0x007B88E0
// Head of the free U queue, 2500 when the queue is empty.
int g_nNetflowQueueHead = 0;

// One slot of the augmenting path search queue.
struct NetflowQueueEntry {
    int mUVertex; // +0x00: U vertex held in this slot.
    int mParent;  // +0x04: parent queue index, -1 for a root.
    int mVVertex; // +0x08: V vertex used to reach this slot.
};

// 0x007B88E8
// The search queue, 2500 slots of twelve bytes.
NetflowQueueEntry g_aNetflowQueue[NETFLOW_MAX_VERTICES];

// 0x007BFE18
// The U vertices on the current search path.
unsigned char g_abNetflowVisited[NETFLOW_MAX_VERTICES];

// 0x005e6508
// Clear every adjacency list and mate, and clear the U count.
void netflow_graph_init(struct netflow_graph *graph) {
    for (int i = 0; i < NETFLOW_MAX_VERTICES; ++i) {
        graph->u[i].edges = nullptr;
        graph->u[i].mate = 0;
    }
    graph->u_count = 0;
}

// 0x005e64e8
// Clear every mate, and clear the V count.
void netflow_v_side_init(struct netflow_v_side *side) {
    for (int i = 0; i < NETFLOW_MAX_VERTICES; ++i) {
        side->mate[i] = 0;
    }
    side->v_count = 0;
}

// 0x005e6538
// Prepend a record for the edge, and allocate a reverse record recording u.
void netflow_add_edge(int u, int v, struct netflow_graph *graph, struct netflow_v_side *side) {
    (void)side;
    struct netflow_edge *pForward = static_cast<struct netflow_edge *>(NetflowAllocChecked(8));
    struct netflow_edge *pReverse = static_cast<struct netflow_edge *>(NetflowAllocChecked(8));
    pForward->next = graph->u[u].edges;
    graph->u[u].edges = pForward;
    pForward->v = v;
    pReverse->v = u;
}

// 0x00569bf0
// Seed a greedy matching, initialise the pool, and grow along augmenting paths.
void netflow_build_matching(struct netflow_graph *graph, struct netflow_v_side *side) {
    NetflowReportInitialMatching(graph, side);
    NetflowInitMatchingPool(graph, side);
    NetflowFindAugmentingPaths(graph, side);
}

// 0x00610880
// Reports an empty edge pool and returns.
void NetflowReportOutOfSpace(const char *pszMessage) {
    fprintf(stderr, "%s\n", pszMessage);
}

// 0x00610840
// Allocates an edge record, reporting an empty pool. The image routes the request through the
// shared reentrancy allocator; the toolchain allocator serves the same pool here.
void *NetflowAllocChecked(unsigned nSize) {
    void *pMemory = malloc(nSize);
    if (pMemory == nullptr) {
        NetflowReportOutOfSpace("Out of space");
    }
    return pMemory;
}

// 0x00569c38
// Matches every U vertex with its first free neighbour and reports the count.
void NetflowReportInitialMatching(struct netflow_graph *graph, struct netflow_v_side *side) {
    int nMatched = 0;
    for (int u = 1; u <= graph->u_count; ++u) {
        struct netflow_edge *pEdge = graph->u[u].edges;
        while (pEdge != nullptr && side->mate[pEdge->v] != 0) {
            pEdge = pEdge->next;
        }
        if (pEdge != nullptr) {
            side->mate[pEdge->v] = u;
            graph->u[u].mate = pEdge->v;
            ++nMatched;
        }
    }
    const double dPercent = static_cast<double>(static_cast<float>(nMatched) * 100.0f /
                                                static_cast<float>(graph->u_count));
    LogPrintf("%d vertices in U were initially matched (%.1f%%).\n", nMatched, dPercent);
}

// 0x0062be30
// Clears the search statistics and queues every unmatched U vertex.
void NetflowInitMatchingPool(struct netflow_graph *graph, struct netflow_v_side *side) {
    (void)side;
    g_nNetflowProbeCount = 0;
    g_nNetflowAugmentTotal = 0;
    int nSlot = NETFLOW_MAX_VERTICES;
    for (int u = 1; u <= graph->u_count; ++u) {
        // The reserved field stores the resume pointer for the search.
        graph->u[u].reserved = reinterpret_cast<int>(graph->u[u].edges);
        g_abNetflowVisited[u] = 0;
        if (graph->u[u].mate == 0) {
            --nSlot;
            g_aNetflowQueue[nSlot].mUVertex = u;
        }
    }
    g_nNetflowQueueHead = nSlot;
}

// 0x0062bb38
// Grows the matching along augmenting paths from every queued U vertex.
void NetflowFindAugmentingPaths(struct netflow_graph *graph, struct netflow_v_side *side) {
    while (g_nNetflowQueueHead < NETFLOW_MAX_VERTICES) {
        const int nRoot = g_aNetflowQueue[g_nNetflowQueueHead].mUVertex;
        ++g_nNetflowQueueHead;
        g_aNetflowQueue[0].mUVertex = nRoot;
        g_aNetflowQueue[0].mParent = -1;
        int nWrite = 0;
        int nRead = 0;
        int nParent = -1;
        NetflowQueueEntry *pSlot = g_aNetflowQueue;
        for (;;) {
            const int nNode = pSlot->mUVertex;
            ++nParent;
            ++nRead;
            ++pSlot;
            ++g_nNetflowProbeCount;
            struct netflow_edge *pEdge = graph->u[nNode].edges;
            if (pEdge != nullptr) {
                do {
                    ++g_nNetflowProbeCount;
                    const int nVertex = pEdge->v;
                    const int nMatched = side->mate[nVertex];
                    if (g_abNetflowVisited[nMatched] == 0) {
                        ++g_nNetflowProbeCount;
                        ++nWrite;
                        g_aNetflowQueue[nWrite].mUVertex = nMatched;
                        g_aNetflowQueue[nWrite].mParent = nParent;
                        g_aNetflowQueue[nWrite].mVVertex = nVertex;
                        g_abNetflowVisited[nMatched] = 1;
                        struct netflow_edge *pResume =
                            reinterpret_cast<struct netflow_edge *>(graph->u[nMatched].reserved);
                        while (pResume != nullptr && side->mate[pResume->v] != 0) {
                            ++g_nNetflowProbeCount;
                            pResume = pResume->next;
                        }
                        graph->u[nMatched].reserved = reinterpret_cast<int>(pResume);
                        if (pResume != nullptr) {
                            const int nFree = pResume->v;
                            side->mate[nFree] = nMatched;
                            graph->u[nMatched].mate = nFree;
                            int nChild = nWrite;
                            int nUp = g_aNetflowQueue[nChild].mParent;
                            while (nUp >= 0) {
                                const int nUpVertex = g_aNetflowQueue[nChild].mVVertex;
                                const int nUpMate = g_aNetflowQueue[nUp].mUVertex;
                                side->mate[nUpVertex] = nUpMate;
                                graph->u[nUpMate].mate = nUpVertex;
                                nChild = nUp;
                                nUp = g_aNetflowQueue[nChild].mParent;
                            }
                            ++g_nNetflowAugmentTotal;
                            for (int i = 0; i <= nWrite; ++i) {
                                g_abNetflowVisited[g_aNetflowQueue[i].mUVertex] = 0;
                            }
                        }
                    }
                    pEdge = pEdge->next;
                } while (pEdge != nullptr);
            }
            if (nWrite >= nRead) {
                continue;
            }
            break;
        }
    }
}
