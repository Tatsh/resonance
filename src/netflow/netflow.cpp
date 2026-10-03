#include "netflow/netflow.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include "os/log.h"

extern "C" {

// The edge builder requests eight byte records from this allocator.
void *Alloc(unsigned nSize);

// Reports an empty edge pool.
void Barf(const char *pszMessage);

// The matcher runs this greedy pass first and reports the result.
void NetflowReportInitialMatching(struct netflow_graph *graph, struct netflow_v_side *side);

// The matcher runs this pool pass between the greedy pass and the search.
void NetflowInitMatchingPool(struct netflow_graph *graph, struct netflow_v_side *side);

// The matcher runs this augmenting path search last.
void NetflowFindAugmentingPaths(struct netflow_graph *graph, struct netflow_v_side *side);

} // extern "C"

// NTSC-U/C: 0x007b88d8, PAL: 0x007fc5d8
// Edge examinations counted across the search, statistics only.
int eq = 0;

// NTSC-U/C: 0x007b88dc, PAL: 0x007fc5dc
// Augmentations applied, statistics only.
int eex = 0;

// NTSC-U/C: 0x007b88e0, PAL: 0x007fc5e0
// Head of the free U queue, 2500 when the queue is empty.
int iUnmatched = 0;

// One slot of the augmenting path search queue.
struct NetflowQueueEntry {
    int mUVertex; // +0x00: U vertex held in this slot.
    int mParent;  // +0x04: parent queue index, -1 for a root.
    int mVVertex; // +0x08: V vertex used to reach this slot.
};

// NTSC-U/C: 0x007b88e8, PAL: 0x007fc5e8
// The search queue, 2500 slots of twelve bytes.
NetflowQueueEntry queue[NETFLOW_MAX_VERTICES];

// NTSC-U/C: 0x007bfe18, PAL: 0x00803b18
// The U vertices on the current search path.
unsigned char visited[NETFLOW_MAX_VERTICES];

// NTSC-U/C: 0x005e6508, PAL: 0x006286f0
// Clear every adjacency list and mate, and clear the U count.
void Init_U(struct netflow_graph *graph) {
    for (int i = 0; i < NETFLOW_MAX_VERTICES; ++i) {
        graph->u[i].edges = nullptr;
        graph->u[i].mate = 0;
    }
    graph->u_count = 0;
}

// NTSC-U/C: 0x005e64e8, PAL: 0x006286d0
// Clear every mate, and clear the V count.
void Init_V(struct netflow_v_side *side) {
    for (int i = 0; i < NETFLOW_MAX_VERTICES; ++i) {
        side->mate[i] = 0;
    }
    side->v_count = 0;
}

// NTSC-U/C: 0x005e6538, PAL: 0x00628720
// Prepend a record for the edge, and allocate a reverse record recording u.
void AddEdge(int u, int v, struct netflow_graph *graph, struct netflow_v_side *side) {
    (void)side;
    struct netflow_edge *pForward = static_cast<struct netflow_edge *>(Alloc(8));
    struct netflow_edge *pReverse = static_cast<struct netflow_edge *>(Alloc(8));
    pForward->next = graph->u[u].edges;
    graph->u[u].edges = pForward;
    pForward->v = v;
    pReverse->v = u;
}

// NTSC-U/C: 0x00569bf0, PAL: 0x005aa0b8
// Seed a greedy matching, initialise the pool, and grow along augmenting paths.
void Match(struct netflow_graph *graph, struct netflow_v_side *side) {
    NetflowReportInitialMatching(graph, side);
    NetflowInitMatchingPool(graph, side);
    NetflowFindAugmentingPaths(graph, side);
}

// NTSC-U/C: 0x00610880, PAL: 0x006514f0
// Reports an empty edge pool and returns.
void Barf(const char *pszMessage) {
    fprintf(stderr, "%s\n", pszMessage);
}

// NTSC-U/C: 0x00610840, PAL: 0x006514b0
// Allocates an edge record, reporting an empty pool. The image routes the request through the
// shared reentrancy allocator; the toolchain allocator serves the same pool here.
void *Alloc(unsigned nSize) {
    void *pMemory = malloc(nSize);
    if (pMemory == nullptr) {
        Barf("Out of space");
    }
    return pMemory;
}

// NTSC-U/C: 0x00569c38, PAL: 0x005aa100
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
    printf("%d vertices in U were initially matched (%.1f%%).\n", nMatched, dPercent);
}

// NTSC-U/C: 0x0062be30, PAL: 0x0066c9c0
// Clears the search statistics and queues every unmatched U vertex.
void NetflowInitMatchingPool(struct netflow_graph *graph, struct netflow_v_side *side) {
    (void)side;
    eq = 0;
    eex = 0;
    int nSlot = NETFLOW_MAX_VERTICES;
    for (int u = 1; u <= graph->u_count; ++u) {
        // The reserved field stores the resume pointer for the search.
        graph->u[u].reserved = reinterpret_cast<int>(graph->u[u].edges);
        visited[u] = 0;
        if (graph->u[u].mate == 0) {
            --nSlot;
            queue[nSlot].mUVertex = u;
        }
    }
    iUnmatched = nSlot;
}

// NTSC-U/C: 0x0062bb38, PAL: 0x0066c6c8
// Grows the matching along augmenting paths from every queued U vertex.
void NetflowFindAugmentingPaths(struct netflow_graph *graph, struct netflow_v_side *side) {
    while (iUnmatched < NETFLOW_MAX_VERTICES) {
        const int nRoot = queue[iUnmatched].mUVertex;
        ++iUnmatched;
        queue[0].mUVertex = nRoot;
        queue[0].mParent = -1;
        int nWrite = 0;
        int nRead = 0;
        int nParent = -1;
        NetflowQueueEntry *pSlot = queue;
        for (;;) {
            const int nNode = pSlot->mUVertex;
            ++nParent;
            ++nRead;
            ++pSlot;
            ++eq;
            struct netflow_edge *pEdge = graph->u[nNode].edges;
            if (pEdge != nullptr) {
                do {
                    ++eq;
                    const int nVertex = pEdge->v;
                    const int nMatched = side->mate[nVertex];
                    if (visited[nMatched] == 0) {
                        ++eq;
                        ++nWrite;
                        queue[nWrite].mUVertex = nMatched;
                        queue[nWrite].mParent = nParent;
                        queue[nWrite].mVVertex = nVertex;
                        visited[nMatched] = 1;
                        struct netflow_edge *pResume =
                            reinterpret_cast<struct netflow_edge *>(graph->u[nMatched].reserved);
                        while (pResume != nullptr && side->mate[pResume->v] != 0) {
                            ++eq;
                            pResume = pResume->next;
                        }
                        graph->u[nMatched].reserved = reinterpret_cast<int>(pResume);
                        if (pResume != nullptr) {
                            const int nFree = pResume->v;
                            side->mate[nFree] = nMatched;
                            graph->u[nMatched].mate = nFree;
                            int nChild = nWrite;
                            int nUp = queue[nChild].mParent;
                            while (nUp >= 0) {
                                const int nUpVertex = queue[nChild].mVVertex;
                                const int nUpMate = queue[nUp].mUVertex;
                                side->mate[nUpVertex] = nUpMate;
                                graph->u[nUpMate].mate = nUpVertex;
                                nChild = nUp;
                                nUp = queue[nChild].mParent;
                                // Yes, the binary counts every step of the path, not each path.
                                ++eex;
                            }
                            for (int i = 0; i <= nWrite; ++i) {
                                visited[queue[i].mUVertex] = 0;
                            }
                            // The binary abandons this root after one augmentation.
                            nWrite = nParent;
                            break;
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
